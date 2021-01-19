// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

#include <string.h> // for strstr

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileNode::FileNode( const AString & fileName, uint8_t controlFlags )
    : Node( fileName, Node::FILE_NODE, controlFlags )
{
    ASSERT( fileName.EndsWith( "\\" ) == false );
    #if defined( __WINDOWS__ )
        ASSERT( ( fileName.FindLast( ':' ) == nullptr ) ||
                ( fileName.FindLast( ':' ) == ( fileName.Get() + 1 ) ) );
    #endif

    m_LastBuildTimeMs = 1; // very little work required
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool FileNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ )
{
    ASSERT( false ); // Should never get here
    return false;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FileNode::~FileNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult FileNode::DoBuild( Job * /*job*/ )
{
    // NOTE: Not calling RecordStampFromBuiltFile as this is not a built file
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    // Don't assert m_Stamp != 0 as input file might not exist
    return NODE_RESULT_OK;
}

// HandleWarningsMSVC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsMSVC( Job * job, const AString & name, const AString & data )
{
    constexpr const char * msvcWarningString = ": warning ";  // string is ok even in non-English
    return HandleWarnings( job, name, data, msvcWarningString );
}

// HandleWarningsClangCl
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangCl( Job * job, const AString & name, const AString & data )
{
    constexpr const char * clangWarningString = " warning:";
    return HandleWarnings( job, name, data, clangWarningString );
}

// HandleWarnings
//------------------------------------------------------------------------------
void FileNode::HandleWarnings( Job * job, const AString & name, const AString & data, const char * warningString )
{
    if ( data.IsEmpty() )
    {
        return;
    }

    // Are there any warnings?
    if ( data.Find( warningString ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, name, data, treatAsWarnings );
    }
}

// HandleWarningsClangGCC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangGCC( Job * job, const AString & name, const AString & data )
{
    if ( data.IsEmpty() )
    {
        return;
    }

    // Are there any warnings? (string is ok even in non-English)
    if ( data.Find( "warning: " ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, name, data, treatAsWarnings );
    }
}

// DumpOutput
//------------------------------------------------------------------------------
void FileNode::DumpOutput( Job * job, const AString & name, const AString & data, bool treatAsWarnings )
{
    if ( data.IsEmpty() == false )
    {
        Array< AString > exclusions( 2, false );
        exclusions.EmplaceBack( "Note: including file:" );
        exclusions.EmplaceBack( "#line" );

        AStackString<> msg;
        msg.Format( "%s: %s\n", treatAsWarnings ? "WARNING" : "PROBLEM", name.Get() );

        AString finalBuffer( data.GetLength() + msg.GetLength() );
        finalBuffer += msg;
        finalBuffer += data;

        Node::DumpOutput( job, finalBuffer, &exclusions );
    }
}

//------------------------------------------------------------------------------
