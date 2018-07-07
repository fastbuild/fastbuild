// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "UnityNode.h"
#include "DirectoryListNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h" // TODO:C Remove this
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
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
    REFLECT_ARRAY( m_ExcludePatterns,   "UnityInputExcludePattern",             MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_ObjectLists,       "UnityInputObjectLists",                MetaOptional() )
    REFLECT( m_OutputPath,              "UnityOutputPath",                      MetaPath() )
    REFLECT( m_OutputPattern,           "UnityOutputPattern",                   MetaOptional() )
    REFLECT( m_NumUnityFilesToCreate,   "UnityNumFiles",                        MetaOptional() + MetaRange( 1, 1048576 ) )
    REFLECT( m_MaxIsolatedFiles,        "UnityInputIsolateWritableFilesLimit",  MetaOptional() + MetaRange( 0, 1048576 ) )
    REFLECT( m_IsolateWritableFiles,    "UnityInputIsolateWritableFiles",       MetaOptional() )
    REFLECT( m_PrecompiledHeader,       "UnityPCH",                             MetaOptional() + MetaFile( true ) ) // relative
    REFLECT_ARRAY( m_PreBuildDependencyNames,   "PreBuildDependencies",         MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( UnityNode )

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
, m_IsolatedFiles( 0, true )
, m_UnityFileNames( 0, true )
{
    m_InputPattern.Append( AStackString<>( "*.cpp" ) );
    m_LastBuildTimeMs = 100; // higher default than a file node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool UnityNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    Dependencies dirNodes( m_InputPaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph, iter, function, m_InputPaths, m_PathsToExclude, m_FilesToExclude, m_ExcludePatterns, m_InputPathRecurse, &m_InputPattern, "UnityInputPath", dirNodes ) )
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

    ASSERT( m_StaticDependencies.IsEmpty() );
    m_StaticDependencies.Append( dirNodes );
    m_StaticDependencies.Append( objectListNodes );
    m_StaticDependencies.Append( fileNodes );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
UnityNode::~UnityNode()
{
    // cleanup extra FileInfo structures
    for ( auto* info : m_FilesInfo )
    {
        FDELETE info;
    }
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult UnityNode::DoBuild( Job * UNUSED( job ) )
{
    bool hasOutputMessage = false; // print msg first time we actually save a file

    // Ensure dest path exists
    // NOTE: Normally a node doesn't need to worry about this, but because
    // UnityNode outputs files that do not match the node-name (and doesn't
    // inherit from FileNoe), we have to handle it ourselves
    // TODO:C Would be good to refactor things to avoid this special case
    if ( EnsurePathExistsForFile( m_OutputPath ) == false )
    {
        return NODE_RESULT_FAILED; // EnsurePathExistsForFile will have emitted error
    }

    m_UnityFileNames.SetCapacity( m_NumUnityFilesToCreate );

    // get the files
    Array< FileAndOrigin > files( 4096, true );

    if ( !GetFiles( files ) )
    {
        return NODE_RESULT_FAILED; // GetFiles will have emitted an error
    }

    // TODO:A Sort files for consistent ordering across file systems/platforms

    // how many files should go in each unity file?
    const size_t numFiles = files.GetSize();
    float numFilesPerUnity = (float)numFiles / m_NumUnityFilesToCreate;
    float remainingInThisUnity( 0.0 );

    uint32_t numFilesWritten( 0 );

    size_t index = 0;

    const bool noUnity = FBuild::Get().GetOptions().m_NoUnity;

    AString output;
    output.SetReserved( 32 * 1024 );

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
            output += m_PrecompiledHeader;
            output += "\"\r\n\r\n";
        }

        // make sure any remaining files are added to the last unity to account
        // for floating point imprecision

        // determine allocation of includes for this unity file
        Array< FileAndOrigin > filesInThisUnity( 256, true );
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

            if ( isolate )
            {
                numIsolated++;
            }

            // count the file, whether we wrote it or not, to keep unity files stable
            index++;
            numFilesWritten++;
        }

        // write allocation of includes for this unity file
        const FileAndOrigin * const end = filesInThisUnity.End();
        size_t numFilesActuallyIsolatedInThisUnity( 0 );
        for ( const FileAndOrigin * file = filesInThisUnity.Begin(); file != end; ++file )
        {
            // files which are modified (writable) can optionally be excluded from the unity
            bool isolateThisFile = noUnity;
            if ( m_IsolateWritableFiles && ( ( m_MaxIsolatedFiles == 0 ) || ( numIsolated <= m_MaxIsolatedFiles ) ) )
            {
                // is the file writable?
                if ( file->IsReadOnly() == false )
                {
                    isolateThisFile = true;
                }
            }

            if ( isolateThisFile )
            {
                // disable compilation of this file (comment it out)
                m_IsolatedFiles.Append( *file );
                numFilesActuallyIsolatedInThisUnity++;
            }

            // write pragma showing cpp file being compiled to assist resolving compilation errors
            AStackString<> buffer( file->GetName().Get() );
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
            output += file->GetName();
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
                    AutoPtr< char > mem( (char *)ALLOC( fileSize ) );
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
            if ( hasOutputMessage == false )
            {
                AStackString< 512 > buffer( "Uni: " );
                buffer += GetName();
                buffer += '\n';
                FLOG_BUILD_DIRECT( buffer.Get() );
                hasOutputMessage = true;
            }

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

    return NODE_RESULT_OK;
}

// GetFiles
//------------------------------------------------------------------------------
bool UnityNode::GetFiles( Array< FileAndOrigin > & files )
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
                    files.Append( FileAndOrigin( filesIt, dirNode ) );
                }
            }
        }
        else if ( node->GetType() == Node::OBJECT_LIST_NODE )
        {
            ObjectListNode * objListNode = sIt->GetNode()->CastTo< ObjectListNode >();

            // iterate all the files in the object list
            Array< AString > objListFiles;
            objListNode->GetInputFiles( objListFiles );
            for ( const auto& file : objListFiles )
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
                files.Append( FileAndOrigin( fi, nullptr ) );
            }
        }
        else if ( node->IsAFile() )
        {
            FileIO::FileInfo * fi = FNEW( FileIO::FileInfo() );
            m_FilesInfo.Append( fi ); // keep ptr to delete later
            if ( FileIO::GetFileInfo( node->GetName(), *fi ) )
            {
                // only add files that exist
                files.Append( FileAndOrigin( fi, nullptr ) );
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

    return ok;
}

//------------------------------------------------------------------------------
