// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

#include <string.h> // for strstr

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileNode::FileNode( const AString & fileName, uint32_t controlFlags )
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
void FileNode::HandleWarningsMSVC( Job * job, const AString & name, const char * data, uint32_t dataSize )
{
    if ( ( data == nullptr ) || ( dataSize == 0 ) )
    {
        return;
    }

    // Are there any warnings? (string is ok even in non-English)
    if ( strstr( data, ": warning " ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, data, dataSize, name, treatAsWarnings );
    }
}

// HandleWarningsClangGCC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangGCC( Job * job, const AString & name, const char * data, uint32_t dataSize )
{
    if ( ( data == nullptr ) || ( dataSize == 0 ) )
    {
        return;
    }

    // Are there any warnings? (string is ok even in non-English)
    if ( strstr( data, "warning: " ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, data, dataSize, name, treatAsWarnings );
    }
}

// DumpOutput
//------------------------------------------------------------------------------
void FileNode::DumpOutput( Job * job, const char * data, uint32_t dataSize, const AString & name, bool treatAsWarnings )
{
    if ( ( data != nullptr ) && ( dataSize > 0 ) )
    {
        Array< AString > exclusions( 2, false );
        exclusions.EmplaceBack( "Note: including file:" );
        exclusions.EmplaceBack( "#line" );

        AStackString<> msg;
        msg.Format( "%s: %s\n", treatAsWarnings ? "WARNING" : "PROBLEM", name.Get() );

        AutoPtr< char > mem( (char *)ALLOC( dataSize + msg.GetLength() ) );
        memcpy( mem.Get(), msg.Get(), msg.GetLength() );
        memcpy( mem.Get() + msg.GetLength(), data, dataSize );

        Node::DumpOutput( job, mem.Get(), dataSize + msg.GetLength(), &exclusions );
    }
}

//------------------------------------------------------------------------------
