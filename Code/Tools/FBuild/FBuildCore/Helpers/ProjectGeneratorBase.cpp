// ProjectGeneratorBase
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuildCore
#include "ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args

// CONSTRUCTOR
//------------------------------------------------------------------------------
ProjectGeneratorBase::ProjectGeneratorBase()
    : m_Folders( 128, true )
    , m_Files( 4096, true )
{
    Folder rootFolder;
    m_Folders.Append( rootFolder );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ProjectGeneratorBase::~ProjectGeneratorBase() = default;

// GetProjectRelativePath
//------------------------------------------------------------------------------
void ProjectGeneratorBase::GetProjectRelativePath( const AString & fileName, AString & shortFileName ) const
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

// GetFolderIndexFor
//------------------------------------------------------------------------------
uint32_t ProjectGeneratorBase::GetFolderIndexFor( const AString & path )
{
    // Get the path exluding the file file or dir
    const char * lastSlash = path.FindLast( NATIVE_SLASH );
    if ( ( lastSlash == nullptr ) || ( lastSlash == path.Get() ) )
    {
        return 0; // no sub-path: put it in the root
    }

    // Search for existing folder
    AStackString<> folderPath( path.Get(), lastSlash );
    for ( const Folder& folder : m_Folders )
    {
        if ( folder.m_Path == folderPath )
        {
            return (uint32_t)( &folder - m_Folders.Begin() ); // Found existing
        }
    }

    // Add new folder(s) recursively
    const uint32_t parentFolderIndex = GetFolderIndexFor( folderPath );

    // Create new folder
    Folder f;
    f.m_Path = folderPath;
    m_Folders.Append( f );
    const uint32_t folderIndex = (uint32_t)( m_Folders.GetSize() - 1 );

    // Add to parent folder
    m_Folders[ parentFolderIndex ].m_Folders.Append( folderIndex );

    return folderIndex;
}

// AddFile
//------------------------------------------------------------------------------
void ProjectGeneratorBase::AddFile( const AString & fileName )
{
    // Handle BasePath
    AStackString<> shortFileName;
    GetProjectRelativePath( fileName, shortFileName );

    // Find existing folder
    const uint32_t folderIndex = GetFolderIndexFor( shortFileName );

    // Add file
    File file;
    file.m_Name = shortFileName;
    file.m_FullPath = fileName;
    file.m_FolderIndex = folderIndex;
    m_Files.Append( file );
    const uint32_t fileIndex = (uint32_t)( m_Files.GetSize() - 1 );

    // Add file to folder
    m_Folders[ folderIndex ].m_Files.Append( fileIndex );
}

// Write
//------------------------------------------------------------------------------
void ProjectGeneratorBase::Write( const char * fmtString, ... )
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
void ProjectGeneratorBase::AddConfig( const AString & name, const Node * targetNode )
{
    Config c;
    c.m_Name = name;
    c.m_TargetNode = targetNode;
    m_Configs.Append( c );
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
            AutoPtr< char > mem( ( char *)ALLOC( oldFileSize ) );
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

    FLOG_BUILD( "%s: %s\n", generatorId, fileName.Get() );

    // ensure path exists (normally handled by framework, but Projects
    // are not necessarily a single file)
    if ( Node::EnsurePathExistsForFile( fileName ) == false )
    {
        FLOG_ERROR( "%s - Invalid path for '%s' (error: %u)", generatorId, fileName.Get(), Env::GetLastErr() );
        return false;
    }

    // actually write
    FileStream f;
    if ( !f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "%s - Failed to open '%s' for write (error: %u)", generatorId, fileName.Get(), Env::GetLastErr() );
        return false;
    }
    if ( f.Write( content.Get(), content.GetLength() ) != content.GetLength() )
    {
        FLOG_ERROR( "%s - Error writing to '%s' (error: %u)", generatorId, fileName.Get(), Env::GetLastErr() );
        return false;
    }
    f.Close();

    return true;
}

// GetDefaultAllowedFileExtensions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::GetDefaultAllowedFileExtensions( Array< AString > & extensions )
{
    static const char * defaultExtensions[] =   {
                                        "*.cpp", "*.hpp", "*.cxx", "*.hxx", "*.c",   "*.h",  "*.cc",   "*.hh",
                                        "*.cp",  "*.hp",  "*.cs",  "*.inl", "*.bff", "*.rc", "*.resx", "*.m",  "*.mm",
                                        "*.cu",
                                        "*.asm", "*.s",
                                        "*.natvis" };
    extensions.SetCapacity( sizeof( defaultExtensions ) / sizeof( char * ) );
    for ( auto & ext : defaultExtensions )
    {
        extensions.Append( AStackString<>( ext ) );
    }
}

// FixupAllowedFileExtensions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::FixupAllowedFileExtensions( Array< AString > & extensions )
{
    // For backwards compatibility, we support explicit extensions ".ext" and wildcards "*.ext"
    // To normalize run-time behaviour, we convert everything to wildcard format

    // convert any that are not wildcards patterns
    for ( auto & ext : extensions )
    {
        if ( ext.Find('*') || ext.Find('?') )
        {
            continue; // already a pattern, leave as is
        }

        // convert ".ext" to "*.ext"
        AStackString<> tmp;
        tmp.Format("*%s", ext.Get());
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

// ExtractIntellisenseOptions
//------------------------------------------------------------------------------
/*static*/ void ProjectGeneratorBase::ExtractIntellisenseOptions( const AString & compilerArgs,
                                                                  const char * option,
                                                                  const char * alternateOption,
                                                                  Array< AString > & outOptions,
                                                                  bool escapeQuotes )
{
    ASSERT( option );
    Array< AString > tokens;
    compilerArgs.Tokenize( tokens );

    const size_t optionLen = AString::StrLen( option );
    const size_t alternateOptionLen = alternateOption ? AString::StrLen( alternateOption ) : 0;

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
        if ( ( token == option ) || ( token == alternateOption ) )
        {
            // Handle an incomplete token at the end of list
            if ( i == ( tokens.GetSize() - 1 ) )
            {
                break;
            }

            // Use next token
            optionBody = tokens[ i + 1 ];
        }
        else if ( token.BeginsWith( option ) )
        {
            // use everything after token
            optionBody.Assign( token.Get() + optionLen );
        }
        else if ( alternateOption && token.BeginsWith( alternateOption ) )
        {
            // use everything after token
            optionBody.Assign( token.Get() + alternateOptionLen );
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

//------------------------------------------------------------------------------
