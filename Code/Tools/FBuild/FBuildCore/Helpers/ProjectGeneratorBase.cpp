// ProjectGeneratorBase
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuildCore
#include "ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"

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
void ProjectGeneratorBase::AddConfig( const AString & name, const AString & target )
{
    Config c;
    c.m_Name = name;
    c.m_Target = target;
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

//------------------------------------------------------------------------------
