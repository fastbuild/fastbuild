// ListDependenciesNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ListDependenciesNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/Array.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

#include <stdio.h>

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ListDependenciesNode, Node, MetaName( "Dest" ) + MetaFile() )
    REFLECT(        m_Source,                   "Source",                   MetaFile() + MetaAllowNonFile() )
    REFLECT(        m_Dest,                     "Dest",                     MetaFile() )
    REFLECT_ARRAY(  m_Patterns,                 "Patterns",                 MetaOptional() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( ListDependenciesNode )

// FilterFileDependencies
//------------------------------------------------------------------------------
static void FilterFileDependencies( Array< const AString * > * dependencyList , const Array< AString > & patterns , const Dependencies & dependencies )
{
    dependencyList->SetCapacity( dependencyList->GetSize() + dependencies.GetSize() );

    for ( const Dependency & dep : dependencies )
    {
        if ( dep.IsWeak() )
        {
            continue;
        }

        const Node * const depNode = dep.GetNode();

        if ( depNode->IsAFile() )
        {
            if ( dependencyList->Find( &depNode->GetName() ) )
            {
                continue;
            }

            if ( patterns.GetSize() > 0 )
            {
                for ( const AString & pattern : patterns )
                {
                    if ( PathUtils::IsWildcardMatch( pattern.Get(), depNode->GetName().Get() ) )
                    {
                        dependencyList->Append( &depNode->GetName() );
                        break;
                    }
                }
            }
            else
            {
                dependencyList->Append( &depNode->GetName() );
            }
        }
    }
}

// DependencyAscendingCompareIDeref
//------------------------------------------------------------------------------
class DependencyAscendingCompareIDeref
{
public:
    inline bool operator () ( const AString * a, const AString * b ) const
    {
        #if defined( __WINDOWS__ )
            return ( a->CompareI( *b ) < 0 );
        #else
            return ( a->Compare( *b ) < 0 );
        #endif
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
ListDependenciesNode::ListDependenciesNode()
: FileNode( AString::GetEmpty(), Node::FLAG_NONE )
{
    m_Type = Node::LIST_DEPENDENCIES_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool ListDependenciesNode::Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, funcStartIter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Get nodes for Source of dependency list
    if ( !Function::GetNodeList( nodeGraph, funcStartIter, function, ".Source", m_Source, m_StaticDependencies ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ListDependenciesNode::~ListDependenciesNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ListDependenciesNode::DoBuild( Job * /*job*/ )
{
    EmitOutputMessage();

    // Collect all file dependencies
    Array< const AString * > dependencyList;

    for ( const Dependency & source : m_StaticDependencies )
    {
        if ( source.IsWeak() )
        {
            continue;
        }

        for ( const Dependency & dep : source.GetNode()->GetStaticDependencies() )
        {
            if ( !dep.IsWeak() )
            {
                FilterFileDependencies( &dependencyList, m_Patterns , dep.GetNode()->GetStaticDependencies() );
                FilterFileDependencies( &dependencyList, m_Patterns , dep.GetNode()->GetDynamicDependencies() );
            }
        }

        for ( const Dependency & dep : source.GetNode()->GetDynamicDependencies() )
        {
            if ( !dep.IsWeak() )
            {
                FilterFileDependencies( &dependencyList, m_Patterns , dep.GetNode()->GetStaticDependencies() );
                FilterFileDependencies( &dependencyList, m_Patterns , dep.GetNode()->GetDynamicDependencies() );
            }
        }
    }

    // Sort to get stable results
    dependencyList.Sort( DependencyAscendingCompareIDeref{} );

    // Format file content
    AStackString<> fileContents;

    const AString * prevDep = nullptr;
    for ( const AString * depName : dependencyList )
    {
        if ( prevDep && ( *prevDep == *depName ) ) // ignore duplicated entries
        {
            continue;
        }

        prevDep = depName;
        fileContents += *depName;

        #if defined( __WINDOWS__ )
            fileContents += "\r\n";
        #else
            fileContents += '\n';
        #endif
    }

    // Dump to text file
    FileStream stream;
    if ( !stream.Open( m_Name.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "Could not open '%s' for write. Error: %s", GetName().Get(), LAST_ERROR_STR );
        return NODE_RESULT_FAILED;
    }

    const uint64_t nWritten = stream.WriteBuffer( fileContents.Get(), fileContents.GetLength() );
    stream.Close();

    if ( nWritten != fileContents.GetLength() )
    {
        FLOG_ERROR( "Failed to write to '%s'. Error: %s", GetName().Get(), LAST_ERROR_STR );
        return NODE_RESULT_FAILED;
    }

    Node::RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// EmitOutputMessage
//------------------------------------------------------------------------------
void ListDependenciesNode::EmitOutputMessage() const
{
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "DepList: '%s' -> '%s'\n", m_Source.Get(), GetName().Get() );
    }
}

//------------------------------------------------------------------------------
