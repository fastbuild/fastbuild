// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "UnityNode.h"
#include "DirectoryListNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h" // TODO:C Remove this
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( UnityNode, Node, MetaNone() )
    REFLECT_ARRAY( m_InputPaths,        "UnityInputPath",                       MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_PathsToExclude,    "UnityInputExcludePath",                MetaOptional() + MetaPath() )
    REFLECT( m_InputPathRecurse,        "UnityInputPathRecurse",                MetaOptional() )
    REFLECT_ARRAY( m_InputPattern,      "UnityInputPattern",                    MetaOptional() )
    REFLECT_ARRAY( m_Files,             "UnityInputFiles",                      MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_FilesToExclude,    "UnityInputExcludedFiles",              MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_FilesToIsolate,    "UnityInputIsolatedFiles",              MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_ExcludePatterns,   "UnityInputExcludePattern",             MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_ObjectLists,       "UnityInputObjectLists",                MetaOptional() )
    REFLECT( m_OutputPath,              "UnityOutputPath",                      MetaPath() )
    REFLECT( m_OutputPattern,           "UnityOutputPattern",                   MetaOptional() )
    REFLECT( m_NumUnityFilesToCreate,   "UnityNumFiles",                        MetaOptional() + MetaRange( 1, 1048576 ) )
    REFLECT( m_MaxIsolatedFiles,        "UnityInputIsolateWritableFilesLimit",  MetaOptional() + MetaRange( 0, 1048576 ) )
    REFLECT( m_IsolateWritableFiles,    "UnityInputIsolateWritableFiles",       MetaOptional() )
    REFLECT( m_IsolateListFile,         "UnityInputIsolateListFile",            MetaOptional() + MetaFile() )
    REFLECT( m_PrecompiledHeader,       "UnityPCH",                             MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_PreBuildDependencyNames,   "PreBuildDependencies",         MetaOptional() + MetaFile() + MetaAllowNonFile() )
    REFLECT( m_Hidden,                  "Hidden",                               MetaOptional() )
    REFLECT( m_UseRelativePaths_Experimental, "UseRelativePaths_Experimental",  MetaOptional() )

    // Internal state
    REFLECT_ARRAY( m_UnityFileNames,    "UnityFileNames",                       MetaHidden() + MetaIgnoreForComparison() )
    REFLECT_ARRAY_OF_STRUCT( m_IsolatedFiles, "IsolatedFiles", UnityIsolatedFile, MetaHidden() + MetaIgnoreForComparison() )
REFLECT_END( UnityNode )

REFLECT_STRUCT_BEGIN( UnityIsolatedFile, Struct, MetaNone() )
    REFLECT( m_FileName,                "FileName",                             MetaHidden() )
    REFLECT( m_DirListOriginPath,       "DirListOriginPath",                    MetaHidden() )
REFLECT_END( UnityIsolatedFile )

// CONSTRUCTOR (UnityIsolatedFile)
//------------------------------------------------------------------------------
UnityIsolatedFile::UnityIsolatedFile() = default;

// CONSTRUCTOR (UnityIsolatedFile)
//------------------------------------------------------------------------------
UnityIsolatedFile::UnityIsolatedFile( const AString & fileName, const DirectoryListNode * dirListOrigin )
    : m_FileName( fileName )
    , m_DirListOriginPath( dirListOrigin ? dirListOrigin->GetPath() : AString:: GetEmpty() )
{
}

// DESTRUCTOR (FileAndOrigin)
//------------------------------------------------------------------------------
UnityIsolatedFile::~UnityIsolatedFile() = default;

// CONSTRUCTOR (UnityFileAndOrigin)
//------------------------------------------------------------------------------
UnityNode::UnityFileAndOrigin::UnityFileAndOrigin() = default;

// CONSTRUCTOR (UnityFileAndOrigin)
//------------------------------------------------------------------------------
UnityNode::UnityFileAndOrigin::UnityFileAndOrigin( FileIO::FileInfo * info, DirectoryListNode * dirListOrigin )
    : m_Info( info )
    , m_DirListOrigin( dirListOrigin )
{
    // Store the last directory position for use during sorting
    const char * lastSlash = info->m_Name.FindLast( NATIVE_SLASH );
    if ( lastSlash )
    {
        m_LastSlashIndex = (uint32_t)(lastSlash - info->m_Name.Get());
    }
}

// operator < (UnityFileAndOrigin)
//------------------------------------------------------------------------------
bool UnityNode::UnityFileAndOrigin::operator < ( const UnityFileAndOrigin & other ) const
{
    // Sort files before directories and compare case insensitively

    // Are we in a sub-directory?
    if ( m_LastSlashIndex > 0 )
    {
        // Yes - Is other in a directory?
        if ( other.m_LastSlashIndex == 0 )
        {
            return false; // We are in dir, so other comes first
        }

        // Both in dirs - sort by subdir
        size_t sortLen = Math::Min( m_LastSlashIndex, other.m_LastSlashIndex );
        sortLen++; // Include trailing slash, so subdirs that are partial matches are handled correctly
        const int32_t sortOrder = AString::StrNCmpI( GetName().Get(), other.GetName().Get(), sortLen );
        if ( sortOrder != 0 )
        {
            return ( sortOrder < 0 );
        }

        // Is one path a sub path of the other?
        if ( m_LastSlashIndex != other.m_LastSlashIndex )
        {
            return ( m_LastSlashIndex < other.m_LastSlashIndex ); // Shorter (parent) path goes first
        }

        // In the same directory - fall through to sort by filename inside dir
    }
    else
    {
        // No - Is other in a directory?
        if ( other.m_LastSlashIndex > 0 )
        {
            return true; // Other in dir, so we come first
        }

        // Neither in directory - fall though to sort by filename
    }

    // Sort by name in directory
    const size_t sortLen = Math::Min( GetName().GetLength() - m_LastSlashIndex, other.GetName().GetLength() - other.m_LastSlashIndex );
    const char * a = GetName().Get() + m_LastSlashIndex;
    const char * b = other.GetName().Get() + other.m_LastSlashIndex;
    return ( AString::StrNCmpI( a, b, sortLen ) < 0 );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
UnityNode::UnityNode()
    : Node( AString::GetEmpty(), Node::UNITY_NODE, Node::FLAG_NONE )
    , m_InputPathRecurse( true )
    , m_InputPattern( 1, true )
    , m_Files( 0, true )
    , m_OutputPath()
    , m_OutputPattern( "Unity*.cpp" )
    , m_NumUnityFilesToCreate( 1 )
    , m_PrecompiledHeader()
    , m_PathsToExclude( 0, true )
    , m_FilesToExclude( 0, true )
    , m_IsolateWritableFiles( false )
    , m_MaxIsolatedFiles( 0 )
    , m_ExcludePatterns( 0, true )
    , m_UseRelativePaths_Experimental( false )
    , m_IsolatedFiles( 0, true )
    , m_UnityFileNames( 0, true )
{
    m_InputPattern.EmplaceBack( "*.cpp" );
    m_LastBuildTimeMs = 100; // higher default than a file node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool UnityNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    Dependencies dirNodes( m_InputPaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_InputPaths,
                                              m_PathsToExclude,
                                              m_FilesToExclude,
                                              m_ExcludePatterns,
                                              m_InputPathRecurse,
                                              true, // Include Read-Only status change in hash
                                              &m_InputPattern,
                                              "UnityInputPath",
                                              dirNodes ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    Dependencies fileNodes( m_Files.GetSize() );
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_Files, "UnityInputFiles", fileNodes ) )
    {
        return false; // GetFileNodes will have emitted an error
    }

    Dependencies objectListNodes( m_ObjectLists.GetSize() );
    if ( !Function::GetObjectListNodes( nodeGraph, iter, function, m_ObjectLists, "UnityInputObjectLists", objectListNodes ) )
    {
        return false; // GetObjectListNodes will have emitted an error
    }

    Dependencies isolateFileListNodes;
    if ( m_IsolateListFile.IsEmpty() == false )
    {
        if ( !Function::GetFileNode( nodeGraph,
                                     iter,
                                     function,
                                      m_IsolateListFile,
                                     "UnityInputIsolateListFile",
                                     isolateFileListNodes ) )
        {
            return false; // GetFileNode will have emitted an error
        }
    }

    ASSERT( m_StaticDependencies.IsEmpty() );
    m_StaticDependencies.Append( dirNodes );
    m_StaticDependencies.Append( objectListNodes );
    m_StaticDependencies.Append( fileNodes );
    m_StaticDependencies.Append( isolateFileListNodes );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
UnityNode::~UnityNode()
{
    // cleanup extra FileInfo structures
    // (normally cleaned up at end of DoBuild, but we need to clean up here
    // if there was an error)
    for ( FileIO::FileInfo * info : m_FilesInfo )
    {
        FDELETE info;
    }
}


// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool UnityNode::DetermineNeedToBuild( const Dependencies & deps ) const
{
    // Of IsolateWriteableFiles is enabled and files come from a directory list
    // then we'll be triggered for a build and don't need any special logic.
    // However, if we directly depend on files, the Stamp for those files only
    // contains the modification time and thus won't trigger a build by themselves.
    // To work around this, we force an evaluation in that case.
    // It would be good to eliminate this special case in the future.
    if ( m_IsolateWritableFiles && ( m_Files.IsEmpty() == false ) )
    {
        FLOG_BUILD_REASON( "Need to build '%s' (UnityInputIsolateWritableFiles = true & UnityInputFiles not empty)\n", GetName().Get() );
        return true;
    }

    // Check if any output files have been deleted. This special case is required
    // because we output multiple files. It would be good to eliminate this in
    // the future.
    for ( const AString & unityFileName : m_UnityFileNames )
    {
        if ( FileIO::FileExists( unityFileName.Get() ) == false )
        {
            FLOG_BUILD_REASON( "Need to build '%s' (Output '%s' missing)\n", GetName().Get(), unityFileName.Get() );
            return true;
        }
    }

    return Node::DetermineNeedToBuild( deps );
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult UnityNode::DoBuild( Job * /*job*/ )
{
    // Emit build summary message
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        AStackString< 512 > buffer( "Uni: " );
        buffer += GetName();
        buffer += '\n';
        FLOG_OUTPUT( buffer );
    }

    // Clear lists of files as we'll regenerate them
    m_UnityFileNames.Destruct();
    m_IsolatedFiles.Destruct();

    // Ensure dest path exists
    // NOTE: Normally a node doesn't need to worry about this, but because
    // UnityNode outputs files that do not match the node-name (and doesn't
    // inherit from FileNode), we have to handle it ourselves
    // TODO:C Would be good to refactor things to avoid this special case
    if ( EnsurePathExistsForFile( m_OutputPath ) == false )
    {
        return NODE_RESULT_FAILED; // EnsurePathExistsForFile will have emitted error
    }

    m_UnityFileNames.SetCapacity( m_NumUnityFilesToCreate );

    // get the files
    Array< UnityFileAndOrigin > files( 4096, true );

    if ( !GetFiles( files ) )
    {
        return NODE_RESULT_FAILED; // GetFiles will have emitted an error
    }

    FilterForceIsolated( files, m_IsolatedFiles );

    // get isolated files from list
    StackArray< AString > isolatedFilesFromList;
    if ( !GetIsolatedFilesFromList( isolatedFilesFromList ) )
    {
        return NODE_RESULT_FAILED; // GetFiles will have emitted an error
    }

    // how many files should go in each unity file?
    const size_t numFiles = files.GetSize();
    const float numFilesPerUnity = (float)numFiles / (float)m_NumUnityFilesToCreate;
    float remainingInThisUnity( 0.0 );

    uint32_t numFilesWritten( 0 );

    size_t index = 0;

    const bool noUnity = FBuild::Get().GetOptions().m_NoUnity;

    AString output;
    output.SetReserved( 32 * 1024 );

    Array< uint64_t > stamps( m_NumUnityFilesToCreate, false );

    // Includes will be relative to root
    AStackString<> includeBasePath;
    if ( m_UseRelativePaths_Experimental )
    {
        includeBasePath = FBuild::Get().GetOptions().GetWorkingDir();
        PathUtils::EnsureTrailingSlash( includeBasePath );
    }

    // create each unity file
    for ( size_t i=0; i<m_NumUnityFilesToCreate; ++i )
    {
        // add allocation to this unity
        remainingInThisUnity += numFilesPerUnity;

        // header
        output = "// Auto-generated Unity file - do not modify\r\n\r\n";

        // precompiled header
        if ( !m_PrecompiledHeader.IsEmpty() )
        {
            output += "#include \"";
            if ( m_UseRelativePaths_Experimental )
            {
                AStackString<> relativePath;
                PathUtils::GetRelativePath( includeBasePath, m_PrecompiledHeader, relativePath );
                output += relativePath;
            }
            else
            {
                output += m_PrecompiledHeader;
            }
            output += "\"\r\n\r\n";
        }

        // make sure any remaining files are added to the last unity to account
        // for floating point imprecision

        // determine allocation of includes for this unity file
        Array< UnityFileAndOrigin > filesInThisUnity( 256, true );
        uint32_t numIsolated( 0 );
        const bool lastUnity = ( i == ( m_NumUnityFilesToCreate - 1 ) );
        while ( ( remainingInThisUnity > 0.0f ) || lastUnity )
        {
            remainingInThisUnity -= 1.0f; // reduce allocation, but leave rounding

            // handle cases where there's more unity files than source files
            if ( index >= numFiles )
            {
                break;
            }

            filesInThisUnity.Append( files[index ] );

            // files which are modified (writable) can optionally be excluded from the unity
            bool isolate = noUnity;
            if ( m_IsolateWritableFiles )
            {
                // is the file writable?
                if ( files[ index ].IsReadOnly() == false )
                {
                    isolate = true;
                }
            }

            // files read from isolated list are excluded from the unity
            if ( !isolate )
            {
                isolate = ( isolatedFilesFromList.Find( files[ index ].GetName() ) != nullptr );
            }

            if ( isolate )
            {
                numIsolated++;
                filesInThisUnity.Top().SetIsolated( true );

                FLOG_VERBOSE( "Isolate file '%s' from unity\n", files[ index ].GetName().Get() );
            }

            // count the file, whether we wrote it or not, to keep unity files stable
            index++;
            numFilesWritten++;
        }

        // write allocation of includes for this unity file
        const UnityFileAndOrigin * const end = filesInThisUnity.End();
        size_t numFilesActuallyIsolatedInThisUnity( 0 );
        for ( const UnityFileAndOrigin * file = filesInThisUnity.Begin(); file != end; ++file )
        {
            // files which are modified can optionally be excluded from the unity
            bool isolateThisFile = noUnity;
            if ( ( m_MaxIsolatedFiles == 0 ) || ( numIsolated <= m_MaxIsolatedFiles ) )
            {
                // is the file writable?
                if ( file->IsIsolated() )
                {
                    isolateThisFile = true;
                }
            }

            if ( isolateThisFile )
            {
                // disable compilation of this file (comment it out)
                m_IsolatedFiles.EmplaceBack( file->GetName(), file->GetDirListOrigin() );
                numFilesActuallyIsolatedInThisUnity++;
            }

            // Get relative file path
            AStackString<> relativePath;
            if ( m_UseRelativePaths_Experimental )
            {
                PathUtils::GetRelativePath( includeBasePath, file->GetName(), relativePath );
            }

            // write pragma showing cpp file being compiled to assist resolving compilation errors
            AStackString<> buffer( m_UseRelativePaths_Experimental ? relativePath : file->GetName() );
            buffer.Replace( BACK_SLASH, FORWARD_SLASH ); // avoid problems with slashes in generated code
            #if defined( __LINUX__ )
                output += "//"; // TODO:LINUX - Find how to avoid GCC spamming "note:" about use of pragma
            #else
                if ( isolateThisFile )
                {
                    output += "//";
                }
            #endif
            output += "#pragma message( \"";
            output += buffer;
            output += "\" )\r\n";

            // write include
            if ( isolateThisFile )
            {
                output += "//"; // TODO:C Do this only for "real" isolation (not -nounity)
                                // (this would reduce rebuilds, but currently doensn't work
                                //  because the generated unity.cpp changing is critical
                                //  to triggering the rebuild when switching -nounity on/off)
            }
            output += "#include \"";
            if ( m_UseRelativePaths_Experimental )
            {
                output += relativePath;
            }
            else
            {
                output += file->GetName();
            }
            output += "\"\r\n\r\n";
        }
        output += "\r\n";

        // generate the destination unity file name
        AStackString<> unityName( m_OutputPath );
        unityName += m_OutputPattern;
        {
            AStackString<> tmp;
            tmp.Format( "%u", (uint32_t)i + 1 ); // number from 1
            unityName.Replace( "*", tmp.Get() );
        }

        // only keep track of non-empty unity files (to avoid link errors with empty objects)
        if ( filesInThisUnity.GetSize() != numFilesActuallyIsolatedInThisUnity )
        {
            m_UnityFileNames.Append( unityName );
        }

        stamps.Append( xxHash::Calc64( output.Get(), output.GetLength() ) );

        // need to write the unity file?
        bool needToWrite = false;
        FileStream f;
        if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
        {
            needToWrite = true; // clean build forces regeneration
        }
        else
        {
            if ( f.Open( unityName.Get(), FileStream::READ_ONLY ) )
            {
                const size_t fileSize( (size_t)f.GetFileSize() );
                if ( output.GetLength() != fileSize )
                {
                    // output not the same size as the file on disc
                    needToWrite = true;
                }
                else
                {
                    // files the same size - are the contents the same?
                    UniquePtr< char > mem( (char *)ALLOC( fileSize ) );
                    if ( f.Read( mem.Get(), fileSize ) != fileSize )
                    {
                        // problem reading file - try to write it again
                        needToWrite = true;
                    }
                    else
                    {
                        if ( AString::StrNCmp( mem.Get(), output.Get(), fileSize ) != 0 )
                        {
                            // contents differ
                            needToWrite = true;
                        }
                    }
                }
                f.Close();
            }
            else
            {
                // file missing - must create
                needToWrite = true;
            }
        }

        // needs updating?
        if ( needToWrite )
        {
            if ( f.Open( unityName.Get(), FileStream::WRITE_ONLY ) == false )
            {
                FLOG_ERROR( "Failed to create Unity file '%s'", unityName.Get() );
                return NODE_RESULT_FAILED;
            }

            if ( f.Write( output.Get(), output.GetLength() ) != output.GetLength() )
            {
                FLOG_ERROR( "Error writing Unity file '%s'", unityName.Get() );
                return NODE_RESULT_FAILED;
            }

            f.Close();
        }
    }

    // Sanity check that all files were written
    ASSERT( numFilesWritten == numFiles );

    // Calculate final hash to represent generation of Unity files
    ASSERT( stamps.GetSize() == m_NumUnityFilesToCreate );
    m_Stamp = xxHash::Calc64( &stamps[ 0 ], stamps.GetSize() * sizeof( uint64_t ) );

    // cleanup extra FileInfo structures
    for ( FileIO::FileInfo * info : m_FilesInfo )
    {
        FDELETE info;
    }
    m_FilesInfo.Destruct();

    return NODE_RESULT_OK;
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void UnityNode::Migrate( const Node & oldNode )
{
    // Migrate Node level properties
    Node::Migrate( oldNode );

    // Migrate lazily evaluated properties
    const UnityNode * oldUnityNode = oldNode.CastTo< UnityNode >();
    m_IsolatedFiles = oldUnityNode->m_IsolatedFiles;
    m_UnityFileNames = oldUnityNode->m_UnityFileNames;
}

// GetFiles
//------------------------------------------------------------------------------
bool UnityNode::GetFiles( Array< UnityFileAndOrigin > & files )
{
    bool ok = true;

    // automatically exclude the associated CPP file for a PCH (if there is one)
    AStackString<> pchCPP;
    if ( m_PrecompiledHeader.IsEmpty() == false )
    {
        if ( m_PrecompiledHeader.EndsWithI( ".h" ) )
        {
            pchCPP.Assign( m_PrecompiledHeader.Get(), m_PrecompiledHeader.GetEnd() - 1 );
            pchCPP += "cpp";
        }
    }

    const Dependency * const sEnd = m_StaticDependencies.End();
    for ( const Dependency * sIt = m_StaticDependencies.Begin(); sIt != sEnd; ++sIt )
    {
        const Node* node = sIt->GetNode();

        if ( node->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            DirectoryListNode * dirNode = sIt->GetNode()->CastTo< DirectoryListNode >();
            const FileIO::FileInfo * const filesEnd = dirNode->GetFiles().End();

            // filter files in the dir list
            for ( FileIO::FileInfo * filesIt = dirNode->GetFiles().Begin(); filesIt != filesEnd; ++filesIt )
            {
                bool keep = true;

                if ( pchCPP.IsEmpty() == false )
                {
                    if ( PathUtils::PathEndsWithFile( filesIt->m_Name, pchCPP ) )
                    {
                        keep = false;
                    }
                }

                if ( keep )
                {
                    files.EmplaceBack( filesIt, dirNode );
                }
            }
        }
        else if ( node->GetType() == Node::OBJECT_LIST_NODE )
        {
            const ObjectListNode * objListNode = sIt->GetNode()->CastTo< ObjectListNode >();

            // iterate all the files in the object list
            Array< AString > objListFiles;
            objListNode->GetInputFiles( objListFiles );
            for ( const AString & file : objListFiles )
            {
                FileIO::FileInfo * fi = FNEW( FileIO::FileInfo() );
                m_FilesInfo.Append( fi ); // keep ptr to delete later
                fi->m_Name = file;
                fi->m_LastWriteTime = 0;
                #if defined( __WINDOWS__ )
                    fi->m_Attributes = 0xFFFFFFFF; // FILE_ATTRIBUTE_READONLY set
                #else
                    fi->m_Attributes = 0; // No writable bits set
                #endif
                fi->m_Size = 0;
                files.EmplaceBack( fi, nullptr );
            }
        }
        else if ( node->GetName() == m_IsolateListFile )
        {
            // Don't try to compile the UnityInputIsolateListFile
            continue;
        }
        else if ( node->IsAFile() )
        {
            FileIO::FileInfo * fi = FNEW( FileIO::FileInfo() );
            m_FilesInfo.Append( fi ); // keep ptr to delete later
            if ( FileIO::GetFileInfo( node->GetName(), *fi ) )
            {
                // only add files that exist
                files.EmplaceBack( fi, nullptr );
            }
            else
            {
                FLOG_ERROR( "FBuild: Error: Unity missing file: '%s'\n", node->GetName().Get() );
                ok = false;
            }
        }
        else
        {
            ASSERT( false ); // Something is terribly wrong - bad node
        }
    }

    // Sort files so order is consistent (not reliant on OS, file system or locale)
    files.Sort();

    return ok;
}

// FilterForceIsolated
//------------------------------------------------------------------------------
void UnityNode::FilterForceIsolated( Array< UnityFileAndOrigin > & files, Array< UnityIsolatedFile > & isolatedFiles )
{
    if ( m_FilesToIsolate.IsEmpty() )
    {
        return;
    }

    UnityFileAndOrigin * writeIt = files.Begin();
    const UnityFileAndOrigin * readIt = writeIt;

    for ( ; readIt != files.End(); ++readIt )
    {
        bool isolate = false;
        for ( const AString & filename : m_FilesToIsolate )
        {
            if ( PathUtils::PathEndsWithFile( readIt->GetName(), filename ) )
            {
                isolate = true;
                break;
            }
        }

        if ( isolate )
        {
            isolatedFiles.EmplaceBack( readIt->GetName(), readIt->GetDirListOrigin() );
        }
        else if ( writeIt != readIt )
        {
            ASSERT( writeIt < readIt );
            *writeIt = *readIt;
            writeIt++;
        }
        else
        {
            writeIt++;
        }
    }

    files.SetSize( (uint64_t)( writeIt - files.Begin() ) );
}


// EnumerateInputFiles
//------------------------------------------------------------------------------
void UnityNode::EnumerateInputFiles( void (*callback)( const AString & inputFile, const AString & baseDir, void * userData ), void * userData ) const
{
    for ( const Dependency & dep : m_StaticDependencies )
    {
        const Node * node = dep.GetNode();

        if ( node->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            const DirectoryListNode * dln = node->CastTo< DirectoryListNode >();

            const Array< FileIO::FileInfo > & files = dln->GetFiles();
            for ( const FileIO::FileInfo & fi : files )
            {
                callback( fi.m_Name, dln->GetPath(), userData );
            }
        }
        else if ( node->GetType() == Node::OBJECT_LIST_NODE )
        {
            const ObjectListNode * oln = node->CastTo< ObjectListNode >();

            oln->EnumerateInputFiles( callback, userData );
        }
        else if ( node->IsAFile() )
        {
            callback( node->GetName(), AString::GetEmpty(), userData );
        }
        else
        {
            ASSERT( false ); // unexpected node type
        }
    }
}

// GetIsolatedFiles
//------------------------------------------------------------------------------
bool UnityNode::GetIsolatedFilesFromList( Array< AString > & files ) const
{
    if ( m_IsolateListFile.IsEmpty() )
    {
        return true; // No list specified so option is disabled
    }
    
    // Open file
    FileStream input;
    if ( input.Open( m_IsolateListFile.Get() ) == false )
    {
        FLOG_ERROR( "FBuild: Error: Unity can't open isolated list file: '%s'\n", m_IsolateListFile.Get() );
        return false;
    }

    // Read file into memory
    AStackString<> buffer;
    buffer.SetLength( (uint32_t)input.GetFileSize() );
    if ( input.ReadBuffer( buffer.Get(), buffer.GetLength() ) != buffer.GetLength() )
    {
        FLOG_ERROR( "FBuild: Error: Unity failed to read lines from isolated list file: '%s'\n", m_IsolateListFile.Get() );
        return false;
    }

    // Split lines
    buffer.Replace( '\r', '\n' );
    buffer.Tokenize( files, '\n' ); // Will discard empty lines

    FLOG_VERBOSE( "Imported %u isolated files from list '%s'\n", (uint32_t)files.GetSize(), m_IsolateListFile.Get() );

    for ( AString & filename : files )
    {
        // Remove leading/trailing whitespace
        filename.TrimEnd( ' ' );
        filename.TrimEnd( '\t' );
        filename.TrimStart( ' ' );
        filename.TrimStart( '\t' );

        // Convert to canonical path
        NodeGraph::CleanPath( filename );
    }

    return true;
}

//------------------------------------------------------------------------------
