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
                                     bool recurse )
        : GetFilesHelper( patterns )
        , m_ExcludePaths( excludePaths )
        , m_FilesToExclude( filesToExclude )
        , m_ExcludePatterns( excludePatterns )
    {
        m_Recurse = recurse;
    }

    //--------------------------------------------------------------------------
    virtual bool OnDirectory( const AString & path ) override
    {
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

    DirectoryListNodeGetFilesHelper& operator =(DirectoryListNodeGetFilesHelper&) = delete;
protected:
    const Array<AString> & m_ExcludePaths;
    const Array<AString> & m_FilesToExclude;
    const Array<AString> & m_ExcludePatterns;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
DirectoryListNode::DirectoryListNode()
    : Node( Node::DIRECTORY_LIST_NODE )
    , m_Recursive( true )
    , m_IncludeReadOnlyStatusInHash( false )
{
    m_ControlFlags = Node::FLAG_ALWAYS_BUILD;
    m_LastBuildTimeMs = 100;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool DirectoryListNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*iter*/, const Function * /*function*/ )
{
    // ensure name is correctly formatted
    //   path|[patterns]|recursive|readonlyflag|[excludePath]
    ASSERT( m_Name.BeginsWith( m_Path ) );
    ASSERT( m_Name[ m_Path.GetLength() ] == '|' );
    ASSERT( m_Patterns.IsEmpty() || ( m_Name.Find( m_Patterns[ 0 ].Get() ) == m_Name.Get() + m_Path.GetLength() + 1 ) );
    ASSERT( ( m_Recursive && m_Name.Find( "|true|" ) ) ||
            ( !m_Recursive && m_Name.Find( "|false|" ) ) );
    ASSERT( ( m_IncludeReadOnlyStatusInHash && m_Name.Find( "|rw|" ) ) ||
            ( !m_IncludeReadOnlyStatusInHash && m_Name.Find( "||" ) ) );

    // paths must have trailing slash
    ASSERT( m_Path.EndsWith( NATIVE_SLASH ) );

    // make sure exclusion path has trailing slash if provided
    #if defined( ASSERTS_ENABLED )
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
                                               const Array< AString > & excludePaths,
                                               const Array< AString > & excludeFiles,
                                               const Array< AString > & excludePatterns,
                                               AString & result )
{
    ASSERT( path.EndsWith( NATIVE_SLASH ) );
    AStackString<> patternString;
    if ( patterns )
    {
        const size_t numPatterns = patterns->GetSize();
        for ( size_t i=0; i<numPatterns; ++i )
        {
            if ( i > 0 )
            {
                patternString += '<';
            }
            patternString += (*patterns)[ i ];
        }
    }
    result.Format( "%s|%s|%s|%s|", path.Get(),
                                  patternString.Get(),
                                  recursive ? "true" : "false",
                                  includeReadOnlyFlagInHash ? "rw" : "");

    const AString * const end = excludePaths.End();
    for ( const AString * it = excludePaths.Begin(); it!=end; ++it )
    {
        const AString & excludePath = *it;
        ASSERT( excludePath.EndsWith( NATIVE_SLASH ) );
        result += excludePath;
        result += '<';
    }

    if ( !excludeFiles.IsEmpty() )
    {
        result += '|';
        const AString * const endFiles = excludeFiles.End();
        for ( const AString * itFiles = excludeFiles.Begin(); itFiles != endFiles; ++itFiles )
        {
            const AString & excludedFile = *itFiles;
            result += excludedFile;
            result += '<';
        }
    }

    if ( !excludePatterns.IsEmpty() )
    {
        result += '|';
        const AString * const endPatterns = excludePatterns.End();
        for ( const AString * itPatterns = excludePatterns.Begin(); itPatterns != endPatterns; ++itPatterns )
        {
            const AString & excludedPattern = *itPatterns;
            result += excludedPattern;
            result += '<';
        }
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
                                                m_Recursive );
        FileIO::GetFiles( m_Path, helper );

        // Transfer ownership of filtered list
        m_Files = Move( helper.GetFiles() );
    }

    MakePrettyName();

    if ( FLog::ShowVerbose() )
    {
        AStackString<> buffer;
        const size_t numFiles = m_Files.GetSize();
        buffer.AppendFormat( "Dir: '%s' (found %u files)\n",
                             m_Name.Get(),
                             (uint32_t)numFiles );
        for ( size_t i=0; i<numFiles; ++i )
        {
            buffer.AppendFormat( " - %s\n", m_Files[ i ].m_Name.Get() );
        }
        FLOG_VERBOSE( "%s", buffer.Get() );
    }

    // Hash the directory listing to represent the discovered files
    if ( m_Files.IsEmpty() )
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

    const size_t numFiles = m_Files.GetSize();
    prettyName.AppendFormat( ", files: %zu ", numFiles );

    m_PrettyName = prettyName;
}

//------------------------------------------------------------------------------
