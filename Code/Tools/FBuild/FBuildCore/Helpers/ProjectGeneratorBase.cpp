// ProjectGeneratorBase
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args
#include <string.h> // for memcmp

// CONSTRUCTOR
//------------------------------------------------------------------------------
ProjectGeneratorBase::ProjectGeneratorBase()
    : m_Folders( 128, true )
    , m_Files( 4096, true )
{
    m_RootFolder = FNEW( Folder );
    m_Folders.Append( m_RootFolder );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ProjectGeneratorBase::~ProjectGeneratorBase()
{
    for ( File * file : m_Files )
    {
        FDELETE file;
    }
    for ( Folder * folder : m_Folders )
    {
        FDELETE folder;
    }
}

// GetProjectRelativePath_Deprecated
//------------------------------------------------------------------------------
//
// This function assumes that projects are in sub-directories relative to the base path
// which isn't always the case
//
void ProjectGeneratorBase::GetProjectRelativePath_Deprecated( const AString & fileName, AString & shortFileName ) const
{
    // Find the best (longest) matching Base Path
    const AString * bestPath = &AString::GetEmpty();
    for ( const AString & basePath : m_BasePaths )
    {
        // can't be a better match if it's shorter than one we already found
        if ( basePath.GetLength() <= bestPath->GetLength() )
        {
            continue;
        }

        if ( PathUtils::PathBeginsWith( fileName, basePath ) )
        {
            bestPath = &basePath;
        }
    }

    // return everything after the selected Base Path
    shortFileName = fileName.Get() + bestPath->GetLength();
}

// GetFolderFor
//------------------------------------------------------------------------------
ProjectGeneratorBase::Folder * ProjectGeneratorBase::GetFolderFor( const AString & path )
{
    // Get the path exluding the file file or dir
    const char * lastSlash = path.FindLast( NATIVE_SLASH );
    if ( ( lastSlash == nullptr ) || ( lastSlash == path.Get() ) )
    {
        return m_RootFolder; // no sub-path: put it in the root
    }

    // Search for existing folder
    AStackString<> folderPath( path.Get(), lastSlash );
    for ( Folder * folder : m_Folders )
    {
        if ( folder->m_Path == folderPath )
        {
            return folder; // Found existing
        }
    }

    // Add new folder(s) recursively
    Folder * parentFolder = GetFolderFor( folderPath );

    // Create new folder
    Folder * newFolder = FNEW( Folder );
    newFolder->m_Path = folderPath;
    m_Folders.Append( newFolder );

    // Add to parent folder
    parentFolder->m_Folders.Append( newFolder );

    return newFolder;
}

// SortFilesAndFolders
//------------------------------------------------------------------------------
void  ProjectGeneratorBase::SortFilesAndFolders()
{
    // Sort files and bake final indices
    m_Files.SortDeref();
    for ( uint32_t index = 0; index < m_Files.GetSize(); ++index )
    {
        m_Files[ index ]->m_SortedIndex = index;
    }

    // Sort folders and bake final indices
    m_Folders.SortDeref();
    for ( uint32_t index = 0; index < m_Folders.GetSize(); ++index )
    {
        m_Folders[ index ]->m_SortedIndex = index;

        // Sort files in folders
        m_Folders[ index ]->m_Files.SortDeref();

        // Sort child folders as well
        m_Folders[ index ]->m_Folders.SortDeref();
    }
}

// AddFile
//------------------------------------------------------------------------------
void ProjectGeneratorBase::AddFile( const AString & fileName )
{
    // Handle BasePath
    AStackString<> relativePath;
    GetProjectRelativePath_Deprecated( fileName, relativePath );

    // Find existing folder
    Folder * folder = GetFolderFor( relativePath );

    // Get file name with no path
    const char * fileNameOnly = relativePath.FindLast( '/' );
    fileNameOnly = fileNameOnly ? fileNameOnly : relativePath.FindLast( '\\' );
    fileNameOnly = fileNameOnly ? ( fileNameOnly + 1 ) : relativePath.Get();

    // Add file
    File * newFile = FNEW( File );
    newFile->m_FileName = fileNameOnly;
    newFile->m_FullPath = fileName;
    newFile->m_Folder = folder;
    m_Files.Append( newFile );

    // Add file to folder
    folder->m_Files.Append( newFile );
}

// Write
//------------------------------------------------------------------------------
void ProjectGeneratorBase::Write( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString< 1024 > tmp;

    va_list args;
    va_start(args, fmtString);
    tmp.VFormat( fmtString, args );
    va_end( args );

    // resize output buffer in large chunks to prevent re-sizing
    if ( m_Tmp.GetLength() + tmp.GetLength() > m_Tmp.GetReserved() )
    {
        m_Tmp.SetReserved( m_Tmp.GetReserved() + MEGABYTE );
    }

    m_Tmp += tmp;
}

// AddConfig
//------------------------------------------------------------------------------
void ProjectGeneratorBase::AddConfig( const ProjectGeneratorBaseConfig & config )
{
    m_Configs.Append( &config );
}

// WriteIfDifferent
//------------------------------------------------------------------------------
/*static*/ bool ProjectGeneratorBase::WriteIfDifferent( const char * generatorId, const AString & content, const AString & fileName )
{
    bool needToWrite = false;

    FileStream old;
    if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
    {
        needToWrite = true;
    }
    else if ( old.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        needToWrite = true;
    }
    else
    {
        // files differ in size?
        size_t oldFileSize = (size_t)old.GetFileSize();
        if ( oldFileSize != content.GetLength() )
        {
            needToWrite = true;
        }
        else
        {
            // check content
            UniquePtr< char > mem( ( char *)ALLOC( oldFileSize ) );
            if ( old.Read( mem.Get(), oldFileSize ) != oldFileSize )
            {
                FLOG_ERROR( "%s - Failed to read '%s'", generatorId, fileName.Get() );
                return false;
            }

            // compare content
            if ( memcmp( mem.Get(), content.Get(), oldFileSize ) != 0 )
            {
                needToWrite = true;
            }
        }

        // ensure we are closed, so we can open again for write if needed
        old.Close();
    }

    // only save if missing or different
    if ( needToWrite == false )
    {
        return true; // nothing to do.
    }

    return WriteToDisk( generatorId, content, fileName ); // WriteToDisk will emit error if needed
}

// WriteIfMissing
//------------------------------------------------------------------------------
/*static*/ bool ProjectGeneratorBase::WriteIfMissing( const char * generatorId, const AString & content, const AString & fileName )
{
    // Do nothing if the file already exists
    if ( FileIO::FileExists( fileName.Get() ) )
    {
        return true;
    }

    return WriteToDisk( generatorId, content, fileName ); // WriteToDisk will emit error if needed
}

// WriteToDisk
//------------------------------------------------------------------------------
/*static*/ bool ProjectGeneratorBase::WriteToDisk( const char * generatorId, const AString & content, const AString & fileName )
{
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "%s: %s\n", generatorId, fileName.Get() );
    }

    // ensure path exists (normally handled by framework, but Projects
    // are not necessarily a single file)
    if ( Node::EnsurePathExistsForFile( fileName ) == false )
    {
        FLOG_ERROR( "%s - Invalid path. Error: %s Target: '%s'", generatorId, LAST_ERROR_STR, fileName.Get() );
        return false;
    }

    // actually write
    FileStream f;
    if ( !f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "%s - Failed to open file for write. Error: %s Target: '%s'", generatorId, LAST_ERROR_STR, fileName.Get() );
        return false;
    }
    if ( f.Write( content.Get(), content.GetLength() ) != content.GetLength() )
    {
        FLOG_ERROR( "%s - Error writing file. Error: %s Target: '%s'", generatorId, LAST_ERROR_STR, fileName.Get() );
        return false;
    }
    f.Close();

    return true;
}

// GetDefaultAllowedFileExtensions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::GetDefaultAllowedFileExtensions( Array< AString > & extensions )
{
    static const char * const defaultExtensions[] =
    {
        "*.cpp", "*.hpp", "*.cxx", "*.hxx", "*.c",   "*.h",  "*.cc",   "*.hh",
        "*.cp",  "*.hp",  "*.cs",  "*.inl", "*.bff", "*.rc", "*.resx", "*.m",  "*.mm",
        "*.cu",
        "*.asm", "*.s",
        "*.natvis", "*.editorconfig"
    };
    extensions.SetCapacity( sizeof( defaultExtensions ) / sizeof( char * ) );
    for ( const char * const ext : defaultExtensions )
    {
        extensions.EmplaceBack( ext );
    }
}

// FixupAllowedFileExtensions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::FixupAllowedFileExtensions( Array< AString > & extensions )
{
    // For backwards compatibility, we support explicit extensions ".ext" and wildcards "*.ext"
    // To normalize run-time behaviour, we convert everything to wildcard format

    // convert any that are not wildcards patterns
    for ( AString & ext : extensions )
    {
        if ( ext.Find('*') || ext.Find('?') )
        {
            continue; // already a pattern, leave as is
        }

        // convert ".ext" to "*.ext"
        AStackString<> tmp;
        tmp.Format( "*%s", ext.Get() );
        ext = tmp;
    }
}

// FindTargetForIntellisenseInfo
//------------------------------------------------------------------------------
/*static*/ const ObjectListNode * ProjectGeneratorBase::FindTargetForIntellisenseInfo( const Node * node )
{
    if ( node )
    {
        switch ( node->GetType() )
        {
            case Node::OBJECT_LIST_NODE: return node->CastTo< ObjectListNode >();
            case Node::LIBRARY_NODE: return node->CastTo< LibraryNode >();
            case Node::EXE_NODE:
            {
                // For Exe use first library
                const ObjectListNode * n = FindTargetForIntellisenseInfo( node->GetStaticDependencies() );
                if ( n )
                {
                    return n;
                }
                break; // Nothing found
            }
            case Node::DLL_NODE:
            {
                // For DLL use first library
                const ObjectListNode * n = FindTargetForIntellisenseInfo( node->GetStaticDependencies() );
                if ( n )
                {
                    return n;
                }
                break; // Nothing found
            }
            case Node::TEST_NODE:
            {
                // For test search in executable
                const Node * testExe = node->CastTo< TestNode >()->GetTestExecutable();
                if ( testExe )
                {
                    return FindTargetForIntellisenseInfo( testExe );
                }
                break; // Nothing found
            }
            case Node::ALIAS_NODE:
            {
                const ObjectListNode * n = FindTargetForIntellisenseInfo( node->CastTo< AliasNode >()->GetAliasedNodes() );
                if ( n )
                {
                    return n;
                }
                break; // Nothing aliased - ignore
            }
            case Node::COPY_FILE_NODE:
            {
                return FindTargetForIntellisenseInfo( node->CastTo< CopyFileNode >()->GetSourceNode() );
            }
            default: break; // Unsupported type - ignore
        }
    }
    return nullptr;
}

// FindTargetForIntellisenseInfo
//------------------------------------------------------------------------------
/*static*/ const ObjectListNode * ProjectGeneratorBase::FindTargetForIntellisenseInfo( const Dependencies & deps )
{
    for ( const Dependency & dep : deps )
    {
        const ObjectListNode * n = FindTargetForIntellisenseInfo( dep.GetNode() );
        if ( n )
        {
            return n;
        }
    }
    return nullptr;
}

// ExtractIncludePaths
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ExtractIncludePaths( const AString & compilerArgs,
                                                           Array< AString > & outIncludes,
                                                           bool escapeQuotes )
{
    // Different options add paths to the different groups which are then searched in the order of their priority.
    // So we need to do multiple passes over arguments to get a list of paths in the correct order.
    StackArray< StackArray< AString, 2 >, 5 > prefixes;
    prefixes.SetSize( 5 );
    prefixes[ 0 ].EmplaceBack( "/I" );
    prefixes[ 0 ].EmplaceBack( "-I" );
    prefixes[ 1 ].EmplaceBack( "-isystem-after" ); // NOTE: before -isystem so it's checked first
    prefixes[ 1 ].EmplaceBack( "-isystem" );
    prefixes[ 2 ].EmplaceBack( "/imsvc" );
    prefixes[ 2 ].EmplaceBack( "-imsvc" );
    prefixes[ 3 ].EmplaceBack( "-idirafter" );
    prefixes[ 4 ].EmplaceBack( "-iquote" );

    for ( const StackArray<AString, 2> & group : prefixes )
    {
        const bool keepFullOption = false;
        ExtractIntellisenseOptions( compilerArgs, group, outIncludes, escapeQuotes, keepFullOption );
    }
}

// ExtractDefines
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ExtractDefines( const AString & compilerArgs,
                                                      Array< AString > & outDefines,
                                                      bool escapeQuotes )
{
    StackArray< AString, 2 > prefixes;
    prefixes.EmplaceBack( "/D" );
    prefixes.EmplaceBack( "-D" );

    // Extract various kinds of includes
    const bool keepFullOption = false;
    ExtractIntellisenseOptions( compilerArgs, prefixes, outDefines, escapeQuotes, keepFullOption );
}

// ExtractAdditionalOptions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ExtractAdditionalOptions( const AString & compilerArgs,
                                                                Array< AString > & outOptions )
{
    StackArray< AString, 2 > prefixes;
    prefixes.EmplaceBack( "-std" );
    prefixes.EmplaceBack( "/std" );

    // Extract the options
    const bool escapeQuotes = false;
    const bool keepFullOption = true;
    ExtractIntellisenseOptions( compilerArgs, prefixes, outOptions, escapeQuotes, keepFullOption );
}

// ExtractIntellisenseOptions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ExtractIntellisenseOptions( const AString & compilerArgs,
                                                                  const Array< AString > & prefixes,
                                                                  Array< AString > & outOptions,
                                                                  bool escapeQuotes,
                                                                  bool keepFullOption )
{
    ASSERT( prefixes.IsEmpty() == false );

    Array< AString > tokens;
    compilerArgs.Tokenize( tokens );

    for ( size_t i=0; i<tokens.GetSize(); ++i )
    {
        AString & token = tokens[ i ];

        // strip quotes around token, e.g:    "-IFolder/Folder"
        if ( token.BeginsWith( '"' ) && token.EndsWith( '"' ) )
        {
            token.Assign( token.Get() + 1, token.GetEnd() - 1 );
        }

        AStackString<> optionBody;

        // Handle space between option and payload
        for ( const AString & prefix : prefixes )
        {
            if ( token == prefix )
            {
                // Handle an incomplete token at the end of list
                if ( i == (tokens.GetSize() - 1) )
                {
                    return;
                }

                // Use next token
                optionBody = tokens[ i + 1 ];
                break;
            }
        }

        if ( optionBody.IsEmpty() )
        {
            for ( const AString & prefix : prefixes )
            {
                if ( token.BeginsWith( prefix ) )
                {
                    if ( keepFullOption )
                    {
                        optionBody = token;
                    }
                    else
                    {
                        // use everything after token
                        optionBody.Assign( token.Get() + prefix.GetLength() );
                    }
                    break;
                }
            }
        }

        // Strip quotes around body (e.g. -I"Folder/Folder")
        if ( optionBody.BeginsWith( '"' ) && optionBody.EndsWith( '"' ) )
        {
            optionBody.Trim( 1, 1 );
        }

        // Did we find something?
        if ( optionBody.IsEmpty() == false )
        {
            if ( escapeQuotes )
            {
                optionBody.Replace( "\"", "\\\"" );
            }
            outOptions.Append( optionBody );
        }
    }
}

// ConcatIntellisenseOptions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ConcatIntellisenseOptions( const Array< AString > & tokens,
                                                                 AString & outTokenString,
                                                                 const char* preToken,
                                                                 const char* postToken )
{
    for ( const AString & token : tokens )
    {
        if ( preToken )
        {
            outTokenString += preToken;
        }
        outTokenString += token;
        if ( postToken )
        {
            outTokenString += postToken;
        }
    }
}

// FindExecutableDebugTarget
// - Return the node that would be used for debugging
//------------------------------------------------------------------------------
/*static*/ const FileNode * ProjectGeneratorBase::FindExecutableDebugTarget( const Node * node )
{
    if ( node )
    {
        switch ( node->GetType() )
        {
            case Node::EXE_NODE: return node->CastTo< ExeNode >();
            case Node::DLL_NODE: return node->CastTo< DLLNode >();
            case Node::COPY_FILE_NODE:
            {
                // When copying, we want to debug the copy as that usually means a staging dir
                // is being created (collating dependencies, isolated from intermediate files)
                return (const FileNode *)node;
            }
            case Node::TEST_NODE:
            {
                // Get executable backing test
                return (FileNode *)node->CastTo< TestNode >()->GetTestExecutable();
            }
            case Node::ALIAS_NODE:
            {
                const FileNode * n = FindExecutableDebugTarget( node->CastTo< AliasNode >()->GetAliasedNodes() );
                if ( n )
                {
                    return n;
                }
                break; // Nothing aliased - ignore
            }
            default: break; // Unsupported type - ignore
        }
    }
    
    return nullptr;
}

// FindExecutableDebugTarget
//------------------------------------------------------------------------------
/*static*/ const FileNode * ProjectGeneratorBase::FindExecutableDebugTarget( const Dependencies & deps )
{
    for ( const Dependency & dep : deps )
    {
        const FileNode * n = FindExecutableDebugTarget( dep.GetNode() );
        if ( n )
        {
            return n;
        }
    }
    return nullptr;
}

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::GetRelativePath( const AString & basePath,
                                                       const AString & fileName,
                                                       AString & outRelativeFileName )
{
    AStackString<> cleanFileName;
    #if !defined( __WINDOWS__ )
        // Normally we keep all paths with native slashes, but in this case we
        // have windows slashes, so convert to native for the relative check
        AStackString<> pathCopy( fileName );
        pathCopy.Replace( '\\', '/' );
        NodeGraph::CleanPath( pathCopy, cleanFileName );
    #else
        NodeGraph::CleanPath( fileName, cleanFileName );
    #endif

    PathUtils::GetRelativePath( basePath, cleanFileName, outRelativeFileName );
}

//------------------------------------------------------------------------------
