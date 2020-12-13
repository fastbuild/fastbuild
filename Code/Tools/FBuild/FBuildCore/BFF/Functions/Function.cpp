// Function
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Function.h"
#include "FunctionAlias.h"
#include "FunctionCompiler.h"
#include "FunctionCopy.h"
#include "FunctionCopyDir.h"
#include "FunctionCSAssembly.h"
#include "FunctionDLL.h"
#include "FunctionError.h"
#include "FunctionExec.h"
#include "FunctionExecutable.h"
#include "FunctionForEach.h"
#include "FunctionIf.h"
#include "FunctionLibrary.h"
#include "FunctionListDependencies.h"
#include "FunctionObjectList.h"
#include "FunctionPrint.h"
#include "FunctionRemoveDir.h"
#include "FunctionSettings.h"
#include "FunctionTest.h"
#include "FunctionTextFile.h"
#include "FunctionUnity.h"
#include "FunctionUsing.h"
#include "FunctionVCXProject.h"
#include "FunctionVSProjectExternal.h"
#include "FunctionVSSolution.h"
#include "FunctionXCodeProject.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_AllowNonFile.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_EmbedMembers.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_InheritFromOwner.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/MetaData/Meta_File.h"
#include "Core/Reflection/MetaData/Meta_Hidden.h"
#include "Core/Reflection/MetaData/Meta_Optional.h"
#include "Core/Reflection/MetaData/Meta_Path.h"
#include "Core/Reflection/MetaData/Meta_Range.h"

// system
#include <stdarg.h>
#include <stdio.h>

// Static
//------------------------------------------------------------------------------
/*static*/ Array<const Function *> g_Functions( 25, false );

// CONSTRUCTOR
//------------------------------------------------------------------------------
Function::Function( const char * name )
: m_Name( name )
, m_Seen( false )
, m_AliasForFunction( 256 )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Function::~Function() = default;

// Find
//------------------------------------------------------------------------------
/*static*/ const Function * Function::Find( const AString & name )
{
    for ( const Function * func : g_Functions )
    {
        if ( func->GetName() == name )
        {
            return func;
        }
    }
    return nullptr;
}

// Create
//------------------------------------------------------------------------------
/*static*/ void Function::Create()
{
    g_Functions.Append( FNEW( FunctionAlias ) );
    g_Functions.Append( FNEW( FunctionCompiler ) );
    g_Functions.Append( FNEW( FunctionCopy ) );
    g_Functions.Append( FNEW( FunctionCopyDir ) );
    g_Functions.Append( FNEW( FunctionCSAssembly ) );
    g_Functions.Append( FNEW( FunctionDLL ) );
    g_Functions.Append( FNEW( FunctionError ) );
    g_Functions.Append( FNEW( FunctionExec ) );
    g_Functions.Append( FNEW( FunctionExecutable ));
    g_Functions.Append( FNEW( FunctionForEach ) );
    g_Functions.Append( FNEW( FunctionIf ) );
    g_Functions.Append( FNEW( FunctionLibrary ) );
    g_Functions.Append( FNEW( FunctionListDependencies ) );
    g_Functions.Append( FNEW( FunctionObjectList ) );
    g_Functions.Append( FNEW( FunctionPrint ) );
    g_Functions.Append( FNEW( FunctionRemoveDir ) );
    g_Functions.Append( FNEW( FunctionSettings ) );
    g_Functions.Append( FNEW( FunctionTest ) );
    g_Functions.Append( FNEW( FunctionTextFile ) );
    g_Functions.Append( FNEW( FunctionUnity ) );
    g_Functions.Append( FNEW( FunctionUsing ) );
    g_Functions.Append( FNEW( FunctionVCXProject ) );
    g_Functions.Append( FNEW( FunctionVSProjectExternal ) );
    g_Functions.Append( FNEW( FunctionVSSolution ) );
    g_Functions.Append( FNEW( FunctionXCodeProject ) );
}

// Destroy
//------------------------------------------------------------------------------
/*static*/ void Function::Destroy()
{
    for ( const Function * func : g_Functions )
    {
        FDELETE func;
    }
    g_Functions.Clear();
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool Function::AcceptsHeader() const
{
    return false;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool Function::NeedsHeader() const
{
    return false;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool Function::NeedsBody() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * Function::CreateNode() const
{
    ASSERT( false ); // Should never get here for Functions that have no Node
    return nullptr;
}

// IsUnique
//------------------------------------------------------------------------------
/*virtual*/ bool Function::IsUnique() const
{
    return false;
}

// ParseFunction
//------------------------------------------------------------------------------
/*virtual*/ bool Function::ParseFunction( NodeGraph & nodeGraph,
                                          BFFParser & parser,
                                          const BFFToken * functionNameStart,
                                          const BFFTokenRange & headerRange,
                                          const BFFTokenRange & bodyRange ) const
{
    m_AliasForFunction.Clear();
    if ( AcceptsHeader() && ( headerRange.IsEmpty() == false ) )
    {
        // Check for exactly one string
        const BFFToken * headerArgsIter = headerRange.GetCurrent();
        if ( headerArgsIter->IsString() )
        {
            // Ensure string is not empty
            if ( headerArgsIter->GetValueString().IsEmpty() )
            {
                Error::Error_1003_EmptyStringNotAllowedInHeader( headerArgsIter, this );
                return false;
            }

            // Store alias name for use in Commit
            if ( BFFParser::PerformVariableSubstitutions( headerArgsIter, m_AliasForFunction ) == false )
            {
                return false; // substitution will have emitted an error
            }
        }
        else if ( NeedsHeader() )
        {
            Error::Error_1001_MissingStringStartToken( headerArgsIter, this );
            return false;
        }
        ++headerArgsIter;

        // make sure there are no extraneous tokens
        if ( headerArgsIter != headerRange.GetEnd() )
        {
            Error::Error_1002_MatchingClosingTokenNotFound( headerArgsIter, this, BFFParser::BFF_FUNCTION_ARGS_CLOSE );
            return false;
        }
    }

    // parse the function body
    BFFTokenRange range( bodyRange );
    if ( parser.Parse( range ) == false )
    {
        return false;
    }

    // Take note of how many nodes there are before Commit
    const size_t nodeCountBefore = nodeGraph.GetNodeCount();

    // complete the function
    if (!Commit( nodeGraph, functionNameStart ))
    {
        return false;
    }

    // Check for cyclic dependencies
    const size_t nodeCountAfter = nodeGraph.GetNodeCount();
    if ( nodeCountBefore != nodeCountAfter )
    {
        for ( size_t index = nodeCountBefore; index < nodeCountAfter; ++index )
        {
            const Node* node = nodeGraph.GetNodeByIndex( index );

            const Dependencies * depVectors[ 3 ] = { &node->GetPreBuildDependencies(),
                                                     &node->GetStaticDependencies(),
                                                     &node->GetDynamicDependencies() };
            for ( const Dependencies * depVector : depVectors )
            {
                for ( const Dependency & dep : *depVector )
                {
                    if ( node == dep.GetNode() )
                    {
                        Error::Error_1043_CyclicDependencyDetected( functionNameStart, node->GetName() );
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool Function::Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const
{
    // Create Node
    Node * node = CreateNode();
    ASSERT( node );

    // Get the name
    //  - For nodes that specify a name as a property (usually the output of the node)
    //    use that as the name (i.e. the filename is the name)
    //  - Otherwise, what would normally be the alias
    AStackString<> nameFromMetaData;
    if ( GetNameForNode( nodeGraph, funcStartIter, node->GetReflectionInfoV(), nameFromMetaData ) == false )
    {
        FDELETE node;
        return false; // GetNameForNode will have emitted an error
    }
    const bool aliasUsedForName = nameFromMetaData.IsEmpty();
    const AString & name = ( aliasUsedForName ) ? m_AliasForFunction : nameFromMetaData;
    ASSERT( name.IsEmpty() == false );

    // Check name isn't already used
    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        FDELETE node;
        return false;
    }

    // Set Name
    node->SetName( name );

    // Register with NodeGraph
    nodeGraph.RegisterNode( node );

    // Set properties
    if ( !PopulateProperties( nodeGraph, funcStartIter, node ) )
    {
        return false; // PopulateProperties will have emitted an error
    }

    // Initialize
    if ( !node->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false; // Initialize will have emitted an error
    }

    // If alias was used for name, we're done
    if ( aliasUsedForName )
    {
        return true;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, node );
}

// GetString
//------------------------------------------------------------------------------
bool Function::GetString( const BFFToken * iter, const BFFVariable * & var, const char * name, bool required ) const
{
    ASSERT( name );
    var = nullptr;

    const BFFVariable * v = BFFStackFrame::GetVar( name );

    if ( v == nullptr )
    {
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
            return false;
        }
        return true;
    }

    if ( v->IsString() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( iter, this, name, v->GetType(), BFFVariable::VAR_STRING );
        return false;
    }
    if ( required && v->GetString().IsEmpty() )
    {
        Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, name );
        return false;
    }

    var = v;
    return true;
}

// GetString
//------------------------------------------------------------------------------
bool Function::GetString( const BFFToken * iter, AString & var, const char * name, bool required ) const
{
    const  BFFVariable * stringVar;
    if ( !GetString( iter, stringVar, name, required ) )
    {
        return false; // required and missing
    }
    if ( stringVar )
    {
        var = stringVar->GetString();
    }
    return true;
}

// GetStringOrArrayOfStrings
//------------------------------------------------------------------------------
bool Function::GetStringOrArrayOfStrings( const BFFToken * iter, const BFFVariable * & var, const char * name, bool required ) const
{
    ASSERT( name );
    var = nullptr;

    const BFFVariable * v = BFFStackFrame::GetVar( name );

    if ( v == nullptr )
    {
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
            return false;
        }
        return true;
    }


    // UnityInputPath can be string or array of strings
    if ( v->IsString() || v->IsArrayOfStrings() )
    {
        var = v;
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, name, v->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
    return false;
}

// GetNodeList
//------------------------------------------------------------------------------
bool Function::GetNodeList( NodeGraph & nodeGraph,
                            const BFFToken * iter,
                            const char * propertyName,
                            Dependencies & nodes,
                            bool required,
                            bool allowCopyDirNodes,
                            bool allowUnityNodes,
                            bool allowRemoveDirNodes,
                            bool allowCompilerNodes ) const
{
    ASSERT( propertyName );

    const BFFVariable * var = BFFStackFrame::GetVar( propertyName );
    if ( !var )
    {
        // missing
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( propertyName ) );
            return false; // required!
        }
        return true; // missing but not required
    }

    if ( var->IsArrayOfStrings() )
    {
        // an array of references
        const Array< AString > & nodeNames = var->GetArrayOfStrings();
        nodes.SetCapacity( nodes.GetSize() + nodeNames.GetSize() );
        for ( const AString & nodeName : nodeNames )
        {
            if ( nodeName.IsEmpty() ) // Always an error - because an empty node name is invalid
            {
                Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, propertyName );
                return false;
            }

            if ( !GetNodeList( nodeGraph, iter, this, propertyName, nodeName, nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
            {
                // child func will have emitted error
                return false;
            }
        }
    }
    else if ( var->IsString() )
    {
        if ( var->GetString().IsEmpty() ) // Always an error - because an empty node name is invalid
        {
            Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, propertyName );
            return false;
        }

        if ( !GetNodeList( nodeGraph, iter, this, propertyName, var->GetString(), nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
        {
            // child func will have emitted error
            return false;
        }
    }
    else
    {
        // unsupported type
        Error::Error_1050_PropertyMustBeOfType( iter, this, propertyName, var->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
        return false;
    }

    return true;
}

// GetDirectoryNodeList
//------------------------------------------------------------------------------
/*static*/ bool Function::GetDirectoryListNodeList( NodeGraph & nodeGraph,
                                                    const BFFToken * iter,
                                                    const Function * function,
                                                    const Array< AString > & paths,
                                                    const Array< AString > & excludePaths,
                                                    const Array< AString > & filesToExclude,
                                                    const Array< AString > & excludePatterns,
                                                    bool recurse,
                                                    bool includeReadOnlyStatusInHash,
                                                    const Array< AString > * patterns,
                                                    const char * inputVarName,
                                                    Dependencies & nodes )
{
    // Handle special case of excluded files beginning with ../
    // Since they can be used sensibly by matching just the end
    // of a path, assume they are relative to the working dir.
    // TODO:C Move this during bff parsing when everything is using reflection
    Array< AString > filesToExcludeCleaned( filesToExclude.GetSize(), true );
    for ( const AString& file : filesToExclude )
    {
        AStackString<> cleanPath;
        NodeGraph::CleanPath( file, cleanPath, false );
        if ( cleanPath.BeginsWith( ".." ) )
        {
            NodeGraph::CleanPath( cleanPath );
        }

        filesToExcludeCleaned.Append( cleanPath );
    }

    const AString * const  end = paths.End();
    for ( const AString * it = paths.Begin(); it != end; ++it )
    {
        const AString & path = *it;

        // get node for the dir we depend on
        AStackString<> name;
        DirectoryListNode::FormatName( path,
                                       patterns,
                                       recurse,
                                       includeReadOnlyStatusInHash,
                                       excludePaths,
                                       filesToExcludeCleaned,
                                       excludePatterns,
                                       name );
        Node * node = nodeGraph.FindNode( name );
        if ( node == nullptr )
        {
            node = nodeGraph.CreateDirectoryListNode( name );
            DirectoryListNode * dln = node->CastTo< DirectoryListNode >();
            dln->m_Path = path;
            if ( patterns )
            {
                dln->m_Patterns = *patterns;
            }
            dln->m_Recursive = recurse;
            dln->m_ExcludePaths = excludePaths;
            dln->m_FilesToExclude = filesToExcludeCleaned;
            dln->m_ExcludePatterns = excludePatterns;
            dln->m_IncludeReadOnlyStatusInHash = includeReadOnlyStatusInHash;
            if ( !dln->Initialize( nodeGraph, iter, function ) )
            {
                return false; // Initialize will have emitted an error
            }
        }
        else if ( node->GetType() != Node::DIRECTORY_LIST_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, function, inputVarName, node->GetName(), node->GetType(), Node::DIRECTORY_LIST_NODE );
            return false;
        }

        nodes.EmplaceBack( node );
    }
    return true;
}

// GetCompilerNode
//------------------------------------------------------------------------------
/*static*/ bool Function::GetCompilerNode( NodeGraph & nodeGraph,
                                           const BFFToken * iter,
                                           const Function * function,
                                           const AString & compiler,
                                           CompilerNode * & compilerNode )
{
    Node * cn = nodeGraph.FindNodeExact( compiler );
    compilerNode = nullptr;
    if ( cn != nullptr )
    {
        // Handle resolving to a file which might be created by an implicit compiler
        if ( cn->GetType() == Node::FILE_NODE )
        {
            // Generate a name for the CompilerNode
            AStackString<> implicitCompilerNodeName( "?AutoCompiler?" );
            implicitCompilerNodeName += compiler;
            Node * implicitCompilerNode = nodeGraph.FindNode( implicitCompilerNodeName );
            if ( implicitCompilerNode && ( implicitCompilerNode->GetType() == Node::COMPILER_NODE ) )
            {
                cn = implicitCompilerNode;
            }
        }
        if ( cn->GetType() != Node::COMPILER_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, function, "Compiler", cn->GetName(), cn->GetType(), Node::COMPILER_NODE );
            return false;
        }
        compilerNode = cn->CastTo< CompilerNode >();
    }
    else
    {
        // create a compiler node - don't allow distribution
        // (only explicitly defined compiler nodes can be distributed)
        // set the default executable path to be the compiler exe directory

        // Generate a name for the CompilerNode
        AStackString<> nodeName( "?AutoCompiler?" );
        nodeName += compiler;

        // Check if we've already implicitly created this Compiler
        cn = nodeGraph.FindNode( nodeName );
        if ( cn )
        {
            if ( cn->GetType() != Node::COMPILER_NODE )
            {
                Error::Error_1102_UnexpectedType( iter, function, "Compiler", cn->GetName(), cn->GetType(), Node::COMPILER_NODE );
                return false;
            }
            compilerNode = cn->CastTo< CompilerNode >();
            return true;
        }

        // Implicitly create the new node
        compilerNode = nodeGraph.CreateCompilerNode( nodeName );
        VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "Executable", compiler ) );
        VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "AllowDistribution", false ) );
        const char * lastSlash = compiler.FindLast( NATIVE_SLASH );
        if (lastSlash)
        {
            AStackString<> executableRootPath( compiler.Get(), lastSlash + 1 );
            VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "ExecutableRootPath", executableRootPath ) );
        }
        if ( !compilerNode->Initialize( nodeGraph, iter, nullptr ) )
        {
            return false; // Initialize will have emitted an error
        }
    }

    return true;
}

// GetFileNode
//------------------------------------------------------------------------------
/*static*/ bool Function::GetFileNode( NodeGraph & nodeGraph,
                                       const BFFToken * iter,
                                       const Function * function,
                                       const AString & file,
                                       const char * inputVarName,
                                       Dependencies & nodes )
{
    // get node for the dir we depend on
    Node * node = nodeGraph.FindNode( file );
    if ( node == nullptr )
    {
        node = nodeGraph.CreateFileNode( file );
    }
    else if ( node->IsAFile() == false )
    {
        Error::Error_1005_UnsupportedNodeType( iter, function, inputVarName, node->GetName(), node->GetType() );
        return false;
    }

    nodes.EmplaceBack( node );
    return true;
}

// GetFileNodes
//------------------------------------------------------------------------------
/*static*/ bool Function::GetFileNodes( NodeGraph & nodeGraph,
                                        const BFFToken * iter,
                                        const Function * function,
                                        const Array< AString > & files,
                                        const char * inputVarName,
                                        Dependencies & nodes )
{
    const AString * const  end = files.End();
    for ( const AString * it = files.Begin(); it != end; ++it )
    {
        const AString & file = *it;
        if (!GetFileNode( nodeGraph, iter, function, file, inputVarName, nodes ))
        {
            return false; // GetFileNode will have emitted an error
        }
    }
    return true;
}

// GetObjectListNodes
//------------------------------------------------------------------------------
/*static*/ bool Function::GetObjectListNodes( NodeGraph & nodeGraph,
                                              const BFFToken * iter,
                                              const Function * function,
                                              const Array< AString > & objectLists,
                                              const char * inputVarName,
                                              Dependencies & nodes )
{
    const AString * const  end = objectLists.End();
    for ( const AString * it = objectLists.Begin(); it != end; ++it )
    {
        const AString & objectList = *it;

        // get node for the dir we depend on
        Node * node = nodeGraph.FindNode( objectList );
        if ( node == nullptr )
        {
            Error::Error_1104_TargetNotDefined( iter, function, inputVarName, objectList );
            return false;
        }
        else if ( node->GetType() != Node::OBJECT_LIST_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, function, inputVarName, node->GetName(), node->GetType(), Node::OBJECT_LIST_NODE );
            return false;
        }

        nodes.EmplaceBack( node );
    }
    return true;
}

// GetNodeList
//------------------------------------------------------------------------------
/*static*/ bool Function::GetNodeList( NodeGraph & nodeGraph,
                                       const BFFToken * iter,
                                       const Function * function,
                                       const char * propertyName,
                                       const Array< AString > & nodeNames,
                                       Dependencies & nodes,
                                       bool allowCopyDirNodes,
                                       bool allowUnityNodes,
                                       bool allowRemoveDirNodes,
                                       bool allowCompilerNodes )
{
    // Ok for nodeNames to be empty
    for ( const AString & nodeName : nodeNames )
    {
        ASSERT( nodeName.IsEmpty() == false ); // MetaData should prevent this

        if ( !GetNodeList( nodeGraph, iter, function, propertyName, nodeName, nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
        {
            return false; // GetNodeList will have emitted an error
        }
    }

    return true;
}

// GetNodeList
//------------------------------------------------------------------------------
/*static*/ bool Function::GetNodeList( NodeGraph & nodeGraph,
                                       const BFFToken * iter,
                                       const Function * function,
                                       const char * propertyName,
                                       const AString & nodeName,
                                       Dependencies & nodes,
                                       bool allowCopyDirNodes,
                                       bool allowUnityNodes,
                                       bool allowRemoveDirNodes,
                                       bool allowCompilerNodes )
{
    // get node
    Node * n = nodeGraph.FindNode( nodeName );
    if ( n == nullptr )
    {
        // not found - create a new file node
        n = nodeGraph.CreateFileNode( nodeName );
        nodes.EmplaceBack( n );
        return true;
    }

    // found - is it a file?
    if ( n->IsAFile() )
    {
        // found file - just use as is
        nodes.EmplaceBack( n );
        return true;
    }

    // found - is it an ObjectList?
    if ( n->GetType() == Node::OBJECT_LIST_NODE )
    {
        // use as-is
        nodes.EmplaceBack( n );
        return true;
    }

    // extra types
    if ( allowCopyDirNodes )
    {
        // found - is it an ObjectList?
        if ( n->GetType() == Node::COPY_DIR_NODE )
        {
            // use as-is
            nodes.EmplaceBack( n );
            return true;
        }
    }
    if ( allowRemoveDirNodes )
    {
        // found - is it a RemoveDirNode?
        if ( n->GetType() == Node::REMOVE_DIR_NODE )
        {
            // use as-is
            nodes.EmplaceBack( n );
            return true;
        }
    }
    if ( allowUnityNodes )
    {
        // found - is it a Unity?
        if ( n->GetType() == Node::UNITY_NODE )
        {
            // use as-is
            nodes.EmplaceBack( n );
            return true;
        }
    }
    if ( allowCompilerNodes )
    {
        // found - is it a Compiler?
        if ( n->GetType() == Node::COMPILER_NODE )
        {
            // use as-is
            nodes.EmplaceBack( n );
            return true;
        }
    }

    // found - is it a group?
    if ( n->GetType() == Node::ALIAS_NODE )
    {
        const AliasNode * an = n->CastTo< AliasNode >();
        const Dependencies & aNodes = an->GetAliasedNodes();
        for ( const Dependency * it = aNodes.Begin(); it != aNodes.End(); ++it )
        {
            // TODO:C by passing as string we'll be looking up again for no reason
            const AString & subName = it->GetNode()->GetName();

            if ( !GetNodeList( nodeGraph, iter, function, propertyName, subName, nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
            {
                return false;
            }
        }
        return true;
    }

    // don't know how to handle this type of node
    Error::Error_1005_UnsupportedNodeType( iter, function, propertyName, n->GetName(), n->GetType() );
    return false;
}

// GetStrings
//------------------------------------------------------------------------------
bool Function::GetStrings( const BFFToken * iter, Array< AString > & strings, const char * name, bool required ) const
{
    const BFFVariable * var;
    if ( !GetStringOrArrayOfStrings( iter, var, name, required ) )
    {
        return false; // problem (GetStringOrArrayOfStrings will have emitted error)
    }
    if ( !var )
    {
        ASSERT( !required );
        return true; // not required and not provided: nothing to do
    }

    if ( var->GetType() == BFFVariable::VAR_STRING )
    {
        strings.Append( var->GetString() );
    }
    else if ( var->GetType() == BFFVariable::VAR_ARRAY_OF_STRINGS )
    {
        const Array< AString > & vStrings = var->GetArrayOfStrings();
        strings.Append( vStrings );
    }
    else
    {
        ASSERT( false );
    }
    return true;
}

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( NodeGraph & nodeGraph, const BFFToken * iter, Node * nodeToAlias ) const
{
    Dependencies nodesToAlias( 1, false );
    nodesToAlias.EmplaceBack( nodeToAlias );
    return ProcessAlias( nodeGraph, iter, nodesToAlias );
}

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( NodeGraph & nodeGraph, const BFFToken * iter, Dependencies & nodesToAlias ) const
{
    if ( m_AliasForFunction.IsEmpty() )
    {
        return true; // no alias required
    }

    // check for duplicates
    if ( nodeGraph.FindNode( m_AliasForFunction ) )
    {
        Error::Error_1100_AlreadyDefined( iter, this, m_AliasForFunction );
        return false;
    }

    // create an alias against the node
    AliasNode * an = nodeGraph.CreateAliasNode( m_AliasForFunction );
    an->m_StaticDependencies = nodesToAlias; // TODO: make this use m_Targets & Initialize()

    // clear the string so it can't be used again
    m_AliasForFunction.Clear();

    return true;
}

// GetNameForNode
//------------------------------------------------------------------------------
bool Function::GetNameForNode( NodeGraph & nodeGraph, const BFFToken * iter, const ReflectionInfo * ri, AString & name ) const
{
    // get object MetaData
    const Meta_Name * nameMD = ri->HasMetaData< Meta_Name >();
    if ( nameMD == nullptr )
    {
        return true; // No MetaName, but this is not an error
    }

    // Format "Name" as ".Name" - TODO:C Would be good to eliminate this string copy
    AStackString<> propertyName( "." );
    propertyName += nameMD->GetName();

    // Find the value for this property from the BFF
    const BFFVariable * variable = BFFStackFrame::GetVar( propertyName );
    if ( variable == nullptr )
    {
        Error::Error_1101_MissingProperty( iter, this, propertyName );
        return false;
    }
    if ( variable->IsString() )
    {
        StackArray<AString> strings;
        if ( !PopulateStringHelper( nodeGraph, iter, nullptr, ri->HasMetaData< Meta_File >(), nullptr, variable, strings ) )
        {
            return false; // PopulateStringHelper will have emitted an error
        }

        // Handle empty strings (always required because names can never be empty)
        if ( strings.IsEmpty() || strings[0].IsEmpty() )
        {
            Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, variable->GetName().Get() );
            return false;
        }

        if ( strings.GetSize() != 1 )
        {
            Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
            return false;
        }

        // Check that name isn't already used
        if ( nodeGraph.FindNode( strings[0] ) )
        {
            Error::Error_1100_AlreadyDefined( iter, this, strings[0] );
            return false;
        }

        name = strings[0];
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING );
    return false;
}

// PopulateProperties
//------------------------------------------------------------------------------
bool Function::PopulateProperties( NodeGraph & nodeGraph, const BFFToken * iter, Node * node ) const
{
    const ReflectionInfo * ri = node->GetReflectionInfoV();
    return PopulateProperties( nodeGraph, iter, node, ri );
}

// PopulateProperties
//------------------------------------------------------------------------------
bool Function::PopulateProperties( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectionInfo * ri ) const
{
    do
    {
        const ReflectionIter end = ri->End();
        for ( ReflectionIter it = ri->Begin(); it != end; ++it )
        {
            const ReflectedProperty & property = *it;

            // Don't populate hidden properties
            if ( property.HasMetaData< Meta_Hidden >() )
            {
                continue;
            }

            // Format "Name" as ".Name" - TODO:C Would be good to eliminate this string copy
            AStackString<> propertyName( "." );
            propertyName += property.GetName();

            // Find the value for this property from the BFF
            const BFFVariable * v = BFFStackFrame::GetVar( propertyName );

            if ( !PopulateProperty( nodeGraph, iter, base, property, v ) )
            {
                return false; // PopulateProperty will have emitted an error
            }
        }

        // Traverse into parent class (if there is one)
        ri = ri->GetSuperClass();
    }
    while ( ri );

    return true;
}

// PopulateProperty
//------------------------------------------------------------------------------
bool Function::PopulateProperty( NodeGraph & nodeGraph,
                                 const BFFToken * iter,
                                 void * base,
                                 const ReflectedProperty & property,
                                 const BFFVariable * variable ) const
{
    // Handle MetaEmbedMembers
    if ( property.HasMetaData< Meta_EmbedMembers >() )
    {
        ASSERT( property.GetType() == PropertyType::PT_STRUCT );
        ASSERT( property.IsArray() == false );
        const ReflectedPropertyStruct & rps = static_cast< const ReflectedPropertyStruct & >( property );
        return PopulateProperties( nodeGraph, iter, (Struct *)rps.GetStructBase( base ), rps.GetStructReflectionInfo() );
    }

    // Handle missing but required
    if ( variable == nullptr )
    {
        const bool required = ( property.HasMetaData< Meta_Optional >() == nullptr );
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( property.GetName() ) );
            return false;
        }

        return true; // missing but not required
    }

    const PropertyType pt = property.GetType();
    switch ( pt )
    {
        case PT_ASTRING:
        {
            const bool required = ( property.HasMetaData< Meta_Optional >() == nullptr );
            if ( property.IsArray() )
            {
                return PopulateArrayOfStrings( nodeGraph, iter, base, property, variable, required );
            }
            else
            {
                return PopulateString( nodeGraph, iter, base, property, variable, required );
            }
        }
        case PT_BOOL:
        {
            return PopulateBool( iter, base, property, variable );
        }
        case PT_INT32:
        {
            return PopulateInt32( iter, base, property, variable );
        }
        case PT_UINT32:
        {
            return PopulateUInt32( iter, base, property, variable );
        }
        case PT_STRUCT:
        {
            if ( property.IsArray() )
            {
                return PopulateArrayOfStructs( nodeGraph, iter, base, property, variable );
            }
            break;
        }
        default:
        {
            break;
        }
    }
    ASSERT( false ); // Unsupported type
    return false;
}

// PopulateStringHelper
//------------------------------------------------------------------------------
bool Function::PopulateStringHelper( NodeGraph & nodeGraph, const BFFToken * iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, Array< AString > & outStrings ) const
{
    if ( variable->IsArrayOfStrings() )
    {
        for ( const AString & string : variable->GetArrayOfStrings() )
        {
            if ( !PopulateStringHelper( nodeGraph, iter, pathMD, fileMD, allowNonFileMD, variable, string, outStrings ) )
            {
                return false; // PopulateStringHelper will have emitted an error
            }
        }
        return true;
    }

    if ( variable->IsString() )
    {
        if ( !PopulateStringHelper( nodeGraph, iter, pathMD, fileMD, allowNonFileMD, variable, variable->GetString(), outStrings ) )
        {
            return false; // PopulateStringHelper will have emitted an error
        }
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
    return false;
}

// PopulateStringHelper
//------------------------------------------------------------------------------
bool Function::PopulateStringHelper( NodeGraph & nodeGraph,
                                     const BFFToken * iter,
                                     const Meta_Path * pathMD,
                                     const Meta_File * fileMD,
                                     const Meta_AllowNonFile * allowNonFileMD,
                                     const BFFVariable * variable,
                                     const AString & string,
                                     Array< AString > & outStrings ) const
{
    // Return empty string untouched (expansion of aliases or paths makes no sense)
    if ( string.IsEmpty() )
    {
        outStrings.Append( string );
        return true; // Calling code must determine if this is an error
    }

    // Full paths to files can support aliases
    if ( fileMD && ( !fileMD->IsRelative() ) )
    {
        // Is it an Alias?
        const Node * node = nodeGraph.FindNode( string );
        if ( node )
        {
            if ( node->GetType() == Node::ALIAS_NODE )
            {
                const AliasNode * aliasNode = node->CastTo< AliasNode >();
                for ( const Dependency & aliasedNode : aliasNode->GetAliasedNodes() )
                {
                    if ( !PopulateStringHelper( nodeGraph, iter, pathMD, fileMD, allowNonFileMD, variable, aliasedNode.GetNode()->GetName(), outStrings ) )
                    {
                        return false; // PopulateStringHelper will have emitted an error
                    }
                }
                return true;
            }

            // Handle non-file types (if allowed)
            if ( allowNonFileMD && ( node->IsAFile() == false ) )
            {
                // Are we limited to a specific node type?
                if ( ( allowNonFileMD->IsLimitedToType() == true ) &&
                     ( allowNonFileMD->GetLimitedType() != node->GetType() ) )
                {
                    // Error - node is wrong type
                    Error::Error_1005_UnsupportedNodeType( iter, this, variable->GetName().Get(), node->GetName(), node->GetType() );
                    return false;
                }

                outStrings.Append( string ); // Leave name as-is
                return true;
            }

            // Is the passed in thing a file?
            if ( node->IsAFile() == false )
            {
                Error::Error_1103_NotAFile( iter, this, variable->GetName().Get(), node->GetName(), node->GetType() );
                return false;
            }
        }

        // Fall through to normal file handling
    }

    AStackString<> stringToFix( string );
    if ( !PopulatePathAndFileHelper( iter, pathMD, fileMD, variable->GetName(), stringToFix ) )
    {
        return false; // PopulatePathAndFileHelper will have emitted an error
    }
    outStrings.Append( stringToFix );
    return true;
}

// PopulatePathAndFileHelper
//------------------------------------------------------------------------------
bool Function::PopulatePathAndFileHelper( const BFFToken * iter,
                                          const Meta_Path * pathMD,
                                          const Meta_File * fileMD,
                                          const AString & variableName,
                                          AString & valueToFix ) const
{
    // Calling code must handle empty strings
    ASSERT( valueToFix.IsEmpty() == false );

    // Only one is allowed (having neither is ok too)
    ASSERT( ( fileMD == nullptr ) || ( pathMD == nullptr ) );

    if ( pathMD )
    {
        if ( pathMD->IsRelative() )
        {
            PathUtils::FixupFolderPath( valueToFix );
        }
        else
        {
            // Make absolute, clean slashes and canonicalize
            NodeGraph::CleanPath( valueToFix );

            // Ensure slash termination
            PathUtils::EnsureTrailingSlash( valueToFix );
        }
    }

    if ( fileMD )
    {
        if ( fileMD->IsRelative() )
        {
            PathUtils::FixupFilePath( valueToFix );
        }
        else
        {
            // Make absolute, clean slashes and canonicalize
            NodeGraph::CleanPath( valueToFix );
        }

        // check that a path hasn't been given when we expect a file
        if ( PathUtils::IsFolderPath( valueToFix ) )
        {
            Error::Error_1105_PathNotAllowed( iter, this, variableName.Get(), valueToFix );
            return false;
        }
    }

    return true;
}

// PopulateArrayOfStrings
//------------------------------------------------------------------------------
bool Function::PopulateArrayOfStrings( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const
{
    StackArray<AString> strings;
    if ( !PopulateStringHelper( nodeGraph, iter, property.HasMetaData< Meta_Path >(), property.HasMetaData< Meta_File >(), property.HasMetaData< Meta_AllowNonFile >(), variable, strings ) )
    {
        return false; // PopulateStringHelper will have emitted an error
    }

    // Empty arrays are not allowed if property is required
    if ( strings.IsEmpty() && required )
    {
        Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, property.GetName() ); // TODO:B A specific error for empty array of strings?
        return false;
    }

    // Arrays must not contain empty strings
    for ( const AString& string : strings )
    {
        if ( string.IsEmpty() == true )
        {
            Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, property.GetName() ); // TODO:B A specific error for empty string in array?
            return false;
        }
    }

    property.SetProperty( base, strings );
    return true;
}

// PopulateString
//------------------------------------------------------------------------------
bool Function::PopulateString( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const
{
    StackArray<AString> strings;
    if ( !PopulateStringHelper( nodeGraph, iter, property.HasMetaData< Meta_Path >(), property.HasMetaData< Meta_File >(), property.HasMetaData< Meta_AllowNonFile >(), variable, strings ) )
    {
        return false; // PopulateStringHelper will have emitted an error
    }

    if ( variable->IsString() )
    {
        // Handle empty strings
        if ( strings.IsEmpty() || strings[0].IsEmpty() )
        {
            if ( required )
            {
                Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, variable->GetName().Get() );
                return false;
            }
            else
            {
                property.SetProperty( base, AString::GetEmpty() );
                return true;
            }
        }

        if ( strings.GetSize() != 1 )
        {
            Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
            return false;
        }

        // String to String
        property.SetProperty( base, strings[0] );
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING );
    return false;
}

// PopulateBool
//------------------------------------------------------------------------------
bool Function::PopulateBool( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
{
    if ( variable->IsBool() )
    {
        // Bool to Bool
        property.SetProperty( base, variable->GetBool() );
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_BOOL );
    return false;
}

// PopulateInt32
//------------------------------------------------------------------------------
bool Function::PopulateInt32( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
{
    if ( variable->IsInt() )
    {
        const int32_t value = variable->GetInt();

        // Check range
        const Meta_Range * rangeMD = property.HasMetaData< Meta_Range >();
        if ( rangeMD )
        {
            if ( ( value < rangeMD->GetMin() ) || ( value > rangeMD->GetMax() ) )
            {
                Error::Error_1054_IntegerOutOfRange( iter, this, variable->GetName().Get(), rangeMD->GetMin(), rangeMD->GetMax() );
                return false;
            }
        }

        // Int32
        property.SetProperty( base, value );
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_INT );
    return false;
}

// PopulateUInt32
//------------------------------------------------------------------------------
bool Function::PopulateUInt32( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
{
    if ( variable->IsInt() )
    {
        const int32_t value = variable->GetInt();

        // Check range
        const Meta_Range * rangeMD = property.HasMetaData< Meta_Range >();
        if ( rangeMD )
        {
            if ( ( value < rangeMD->GetMin() ) || ( value > rangeMD->GetMax() ) )
            {
                Error::Error_1054_IntegerOutOfRange( iter, this, variable->GetName().Get(), rangeMD->GetMin(), rangeMD->GetMax() );
                return false;
            }
        }

        // Int32 to UInt32
        property.SetProperty( base, (uint32_t)value );
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_INT );
    return false;
}

// PopulateArrayOfStructs
//------------------------------------------------------------------------------
bool Function::PopulateArrayOfStructs( NodeGraph & nodeGraph,
                                       const BFFToken * iter,
                                       void * base,
                                       const ReflectedProperty & property,
                                       const BFFVariable * variable ) const
{
    // Get the destionation
    const ReflectedPropertyStruct & dstStructs = static_cast< const ReflectedPropertyStruct & >( property );
    ASSERT( dstStructs.IsArray() );

    // Array to Array
    if ( variable->IsArrayOfStructs() )
    {
        // pre-size the destination
        const Array<const BFFVariable *> & srcStructs = variable->GetArrayOfStructs();
        dstStructs.ResizeArrayOfStruct( base, srcStructs.GetSize() );

        // Set the properties of each struct
        size_t index( 0 );
        for ( const BFFVariable * s : srcStructs )
        {
            // Calculate the base for this struct in the array
            void * structBase = dstStructs.GetStructInArray( base, index );

            const ReflectionInfo * ri = dstStructs.GetStructReflectionInfo();
            if ( !PopulateArrayOfStructsElement( nodeGraph, iter, structBase, ri, s ) )
            {
                return false; // PopulateArrayOfStructsElement will have emitted an error
            }

            ++index;
        }
        return true;
    }

    if ( variable->IsStruct() )
    {
        // pre-size the destination
        dstStructs.ResizeArrayOfStruct( base, 1 );

        // Calculate the base for this struct in the array
        void * structBase = dstStructs.GetStructInArray( base, 0 );

        const ReflectionInfo * ri = dstStructs.GetStructReflectionInfo();
        return PopulateArrayOfStructsElement( nodeGraph, iter, structBase, ri, variable ); // Will emit error if needed
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRUCT, BFFVariable::VAR_ARRAY_OF_STRUCTS );
    return false;
}

// PopulateArrayOfStructsElement
//------------------------------------------------------------------------------
bool Function::PopulateArrayOfStructsElement( NodeGraph & nodeGraph,
                                              const BFFToken * iter,
                                              void * structBase,
                                              const ReflectionInfo * structRI,
                                              const BFFVariable * srcVariable ) const
{
    ASSERT( structRI ); // Must be at least one level of reflection
    ASSERT( srcVariable->IsStruct() );

    do
    {
        // Try to populate all the properties for this struct
        for ( ReflectionIter it = structRI->Begin(); it != structRI->End(); ++it )
        {
            const ReflectedProperty & property = *it;

            // Don't populate hidden properties
            if ( property.HasMetaData< Meta_Hidden >() )
            {
                continue;
            }

            AStackString<> propertyName( "." ); // TODO:C Eliminate copy
            propertyName += property.GetName();

            // Try to find property in BFF
            const BFFVariable * const * found = BFFVariable::GetMemberByName( propertyName, srcVariable->GetStructMembers() );
            const BFFVariable * var = nullptr;
            if ( found )
            {
                // Use variable if found
                var = *found;
            }
            else
            {
                // If not found, check for inheritence from containing frame
                if ( property.HasMetaData<Meta_InheritFromOwner>() )
                {
                    var = BFFStackFrame::GetVar( propertyName );
                }
            }
            if ( !PopulateProperty( nodeGraph, iter, structBase, property, var ) )
            {
                return false; // PopulateProperty will have emitted an error
            }
        }

        // Traverse into parent class (if there is one)
        structRI = structRI->GetSuperClass();
    }
    while ( structRI );

    return true;
}

//------------------------------------------------------------------------------
