// DirectoryListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DirectoryListNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( DirectoryListNode, Node, MetaNone() )
    REFLECT( m_Path,                    "Path",             MetaHidden() )
    REFLECT_ARRAY( m_Patterns,          "Patterns",         MetaHidden() )
    REFLECT_ARRAY( m_ExcludePaths,      "ExcludePaths",     MetaHidden() )
    REFLECT_ARRAY( m_FilesToExclude,    "FilesToExclude",   MetaHidden() )
    REFLECT_ARRAY( m_ExcludePatterns,   "ExcludePatterns",  MetaHidden() )
    REFLECT( m_Recursive,               "Recursive",        MetaHidden() )
    REFLECT( m_IncludeReadOnlyStatusInHash, "IncludeReadOnlyStatusInHash", MetaHidden() )
    REFLECT( m_IncludeDirs,             "IncludeDirs",      MetaHidden() )
REFLECT_END( DirectoryListNode )

// DirectoryListNodeGetFilesHelper
//------------------------------------------------------------------------------
class DirectoryListNodeGetFilesHelper : public GetFilesHelper
{
public:
    //--------------------------------------------------------------------------
    DirectoryListNodeGetFilesHelper( const Array<AString> & patterns,
                                     const Array<AString> & excludePaths,
                                     const Array<AString> & filesToExclude,
                                     const Array<AString> & excludePatterns,
                                     bool recurse,
                                     bool includeDirs )
        : GetFilesHelper( patterns )
        , m_ExcludePaths( excludePaths )
        , m_FilesToExclude( filesToExclude )
        , m_ExcludePatterns( excludePatterns )
        , m_IncludeDirs( includeDirs )
    {
        m_Recurse = recurse;
    }

    //--------------------------------------------------------------------------
    virtual bool OnDirectory( const AString & path ) override
    {
        if ( m_IncludeDirs )
        {
            m_Directories.EmplaceBack( path );
        }

        if ( m_Recurse == false )
        {
            return false;
        }

        // Filter excluded paths
        for ( const AString & excludedPath : m_ExcludePaths )
        {
            if ( PathUtils::PathBeginsWith( path, excludedPath ) )
            {
                return false; // Don't recurse into dir
            }
        }

        return true; // Recurse into directory
    }

    //--------------------------------------------------------------------------
    virtual void OnFile( FileIO::FileInfo && info ) override
    {
        // filter excluded files
        for ( const AString & fileToExclude : m_FilesToExclude )
        {
            if ( PathUtils::PathEndsWithFile( info.m_Name, fileToExclude ) )
            {
                return; // Exclude
            }
        }

        // Filter excluded patterns
        for ( const AString & excludePattern : m_ExcludePatterns )
        {
            if ( PathUtils::IsWildcardMatch( excludePattern.Get(), info.m_Name.Get() ) )
            {
                return; // Exclude
            }
        }

        // Keep file info
        m_Files.EmplaceBack( Move( info ) );
    }

    Array<AString>& GetDirectories() { return m_Directories; }

    DirectoryListNodeGetFilesHelper& operator =(DirectoryListNodeGetFilesHelper&) = delete;
protected:
    const Array<AString> & m_ExcludePaths;
    const Array<AString> & m_FilesToExclude;
    const Array<AString> & m_ExcludePatterns;
    const bool m_IncludeDirs;
    Array<AString> m_Directories;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
DirectoryListNode::DirectoryListNode()
    : Node( Node::DIRECTORY_LIST_NODE )
{
    m_ControlFlags = Node::FLAG_ALWAYS_BUILD;
    m_LastBuildTimeMs = 100;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool DirectoryListNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*iter*/, const Function * /*function*/ )
{
    #if defined( ASSERTS_ENABLED )
        // ensure name is correctly formatted
        AStackString<> expectedName;
        FormatName( m_Path,
                    &m_Patterns,
                    m_Recursive,
                    m_IncludeReadOnlyStatusInHash,
                    m_IncludeDirs,
                    m_ExcludePaths,
                    m_FilesToExclude,
                    m_ExcludePatterns,
                    expectedName );
        ASSERT( m_Name == expectedName );

        // paths must have trailing slash
        ASSERT( m_Path.EndsWith( NATIVE_SLASH ) );

        // make sure exclusion path has trailing slash if provided
        for ( const AString & excludePath : m_ExcludePaths )
        {
            ASSERT( excludePath.EndsWith( NATIVE_SLASH ) );
        }
    #endif

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
DirectoryListNode::~DirectoryListNode() = default;

// FormatName
//------------------------------------------------------------------------------
/*static*/ void DirectoryListNode::FormatName( const AString & path,
                                               const Array< AString > * patterns,
                                               bool recursive,
                                               bool includeReadOnlyFlagInHash,
                                               bool includeDirs,
                                               const Array< AString > & excludePaths,
                                               const Array< AString > & excludeFiles,
                                               const Array< AString > & excludePatterns,
                                               AString & result )
{
    ASSERT( path.EndsWith( NATIVE_SLASH ) );

    // Path and pattern
    result = path;
    result += '|';
    if ( patterns )
    {
        result.AppendList( *patterns, '<' );
        result += '|';
    }

    // Additional flags
    if ( recursive )
    {
        result += 'r'; // Recursive
    }
    if ( includeReadOnlyFlagInHash )
    {
        result += 'w'; // Writable flag included in hash
    }
    if ( includeDirs )
    {
        result += 'd'; // Directories included in result
    }

    // Excluded paths
    if ( !excludeFiles.IsEmpty() )
    {
        result += "|ePaths=";
        result.AppendList( excludePaths, '<' );
    }

    // Excluded files
    if ( !excludeFiles.IsEmpty() )
    {
        result += "|eFiles=";
        result.AppendList( excludeFiles, '<' );
    }

    // Excluded patterns
    if ( !excludePatterns.IsEmpty() )
    {
        result += "|ePatterns=";
        result.AppendList( excludePatterns, '<' );
    }
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult DirectoryListNode::DoBuild( Job * /*job*/ )
{
    // NOTE: The DirectoryListNode makes no assumptions about whether no files
    // is an error or not.  That's up to the dependent nodes to decide.

    {
        // Get the list of files, filtered in various ways
        DirectoryListNodeGetFilesHelper helper( m_Patterns,
                                                m_ExcludePaths,
                                                m_FilesToExclude,
                                                m_ExcludePatterns,
                                                m_Recursive,
                                                m_IncludeDirs );
        FileIO::GetFiles( m_Path, helper );

        // Transfer ownership of filtered list
        m_Files = Move( helper.GetFiles() );
        m_Directories = Move( helper.GetDirectories() );
    }

    MakePrettyName();

    if ( FLog::ShowVerbose() )
    {
        AStackString<> buffer;
        buffer.AppendFormat( "Dir: '%s' (%zu files)\n",
                             m_Name.Get(),
                             m_Files.GetSize() );
        for ( const FileIO::FileInfo & file : m_Files )
        {
            buffer.AppendFormat( " - %s\n", file.m_Name.Get() );
        }
        if ( m_IncludeDirs )
        {
            buffer.AppendFormat( "Dir: '%s' (%zu dirs)\n",
                                 m_Name.Get(),
                                 m_Directories.GetSize() );
            for ( const AString & dir : m_Directories )
            {
                buffer.AppendFormat( " - %s\n", dir.Get() );
            }
        }
        FLOG_VERBOSE( "%s", buffer.Get() );
    }

    // Hash the directory listing to represent the discovered files
    if ( m_Files.IsEmpty() && m_Directories.IsEmpty() )
    {
        m_Stamp = 1; // Non-zero
    }
    else
    {
        MemoryStream ms;
        for ( const FileIO::FileInfo & file : m_Files )
        {
            // Include filenames, so additions and removals will change the hash
            ms.WriteBuffer( file.m_Name.Get(), file.m_Name.GetLength() );

            // Include read-only status if desired
            if ( m_IncludeReadOnlyStatusInHash )
            {
                ms.Write( file.IsReadOnly() );
            }
        }
        for ( const AString & dir : m_Directories )
        {
            // additions and removals will change the hash
            ms.WriteBuffer( dir.Get(), dir.GetLength() );
        }
        m_Stamp = xxHash3::Calc64( ms.GetData(), ms.GetSize() );
    }

    return NODE_RESULT_OK;
}

// MakePrettyName
//------------------------------------------------------------------------------
void DirectoryListNode::MakePrettyName()
{
    AStackString<> prettyName( m_Path );
    if (m_Recursive)
    {
        prettyName += " (recursive)";
    }
    if ( m_IncludeReadOnlyStatusInHash )
    {
        prettyName += " (incROStatus)";
    }
    if ( m_IncludeDirs )
    {
        prettyName += " (incDirs)";
    }

    const size_t numFiles = m_Files.GetSize();
    prettyName.AppendFormat( ", files: %zu ", numFiles );

    m_PrettyName = prettyName;
}

//------------------------------------------------------------------------------
