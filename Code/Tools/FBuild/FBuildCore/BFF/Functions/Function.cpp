// Function
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Function.h"
#include "FunctionAlias.h"
#include "FunctionCompiler.h"
#include "FunctionCopy.h"
#include "FunctionCopyDir.h"
#include "FunctionCSAssembly.h"
#include "FunctionDLL.h"
#include "FunctionExec.h"
#include "FunctionExecutable.h"
#include "FunctionForEach.h"
#include "FunctionIf.h"
#include "FunctionLibrary.h"
#include "FunctionObjectList.h"
#include "FunctionPrint.h"
#include "FunctionRemoveDir.h"
#include "FunctionSettings.h"
#include "FunctionSLN.h"
#include "FunctionTest.h"
#include "FunctionUnity.h"
#include "FunctionUsing.h"
#include "FunctionVCXProject.h"
#include "FunctionXCodeProject.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_AllowNonFile.h"
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
/*static*/ Function * Function::s_FirstFunction = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
Function::Function( const char * name )
: m_NextFunction( nullptr )
, m_Name( name )
, m_Seen( false )
, m_AliasForFunction( 256 )
{
    if ( s_FirstFunction == nullptr )
    {
        s_FirstFunction = this;
        return;
    }
    Function * func = s_FirstFunction;
    while ( func )
    {
        if ( func->m_NextFunction == nullptr )
        {
            func->m_NextFunction = this;
            return;
        }
        func = func->m_NextFunction;
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Function::~Function() = default;

// Find
//------------------------------------------------------------------------------
/*static*/ const Function * Function::Find( const AString & name )
{
    Function * func = s_FirstFunction;
    while ( func )
    {
        if ( func->GetName() == name )
        {
            return func;
        }
        func = func->m_NextFunction;
    }
    return nullptr;
}

// Create
//------------------------------------------------------------------------------
/*static*/ void Function::Create()
{
    FNEW( FunctionAlias );
    FNEW( FunctionCompiler );
    FNEW( FunctionCopy );
    FNEW( FunctionCopyDir );
    FNEW( FunctionCSAssembly );
    FNEW( FunctionDLL );
    FNEW( FunctionExec );
    FNEW( FunctionExecutable );
    FNEW( FunctionForEach );
    FNEW( FunctionIf );
    FNEW( FunctionLibrary );
    FNEW( FunctionPrint );
    FNEW( FunctionRemoveDir );
    FNEW( FunctionSettings );
    FNEW( FunctionSLN );
    FNEW( FunctionTest );
    FNEW( FunctionUnity );
    FNEW( FunctionUsing );
    FNEW( FunctionVCXProject );
    FNEW( FunctionObjectList );
    FNEW( FunctionXCodeProject );
}

// Destroy
//------------------------------------------------------------------------------
/*static*/ void Function::Destroy()
{
    Function * func = s_FirstFunction;
    while ( func )
    {
        Function * nextFunc = func->m_NextFunction;
        FDELETE func;
        func = nextFunc;
    }
    s_FirstFunction = nullptr;
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

// IsUnique
//------------------------------------------------------------------------------
/*virtual*/ bool Function::IsUnique() const
{
    return false;
}

// ParseFunction
//------------------------------------------------------------------------------
/*virtual*/ bool Function::ParseFunction( NodeGraph & nodeGraph,
                                          const BFFIterator & functionNameStart,
                                          const BFFIterator * functionBodyStartToken,
                                          const BFFIterator * functionBodyStopToken,
                                          const BFFIterator * functionHeaderStartToken,
                                          const BFFIterator * functionHeaderStopToken ) const
{
    m_AliasForFunction.Clear();
    if ( AcceptsHeader() &&
         functionHeaderStartToken && functionHeaderStopToken &&
         ( functionHeaderStartToken->GetDistTo( *functionHeaderStopToken ) > 1 ) )
    {
        // find opening quote
        BFFIterator start( *functionHeaderStartToken );
        ASSERT( *start == BFFParser::BFF_FUNCTION_ARGS_OPEN );
        start++;
        start.SkipWhiteSpace();
        const char c = *start;
        if ( ( c != '"' ) && ( c != '\'' ) )
        {
            Error::Error_1001_MissingStringStartToken( start, this );
            return false;
        }
        BFFIterator stop( start );
        stop.SkipString( c );
        ASSERT( stop.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated
        if ( start.GetDistTo( stop ) <= 1 )
        {
            Error::Error_1003_EmptyStringNotAllowedInHeader( start, this );
            return false;
        }

        // store alias name for use in Commit
        start++; // skip past opening quote
        if ( BFFParser::PerformVariableSubstitutions( start, stop, m_AliasForFunction ) == false )
        {
            return false; // substitution will have emitted an error
        }
    }

    // parse the function body
    BFFParser subParser( nodeGraph );
    BFFIterator subIter( *functionBodyStartToken );
    subIter++; // skip past opening body token
    subIter.SetMax( functionBodyStopToken->GetCurrent() ); // cap parsing to body end
    if ( subParser.Parse( subIter ) == false )
    {
        return false;
    }

    // complete the function
    return Commit( nodeGraph, functionNameStart );
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool Function::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    (void)nodeGraph;
    (void)funcStartIter;
    return true;
}

// GetString
//------------------------------------------------------------------------------
bool Function::GetString( const BFFIterator & iter, const BFFVariable * & var, const char * name, bool required ) const
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
bool Function::GetString( const BFFIterator & iter, AString & var, const char * name, bool required ) const
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
bool Function::GetStringOrArrayOfStrings( const BFFIterator & iter, const BFFVariable * & var, const char * name, bool required ) const
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

// GetBool
//------------------------------------------------------------------------------
bool Function::GetBool( const BFFIterator & iter, bool & var, const char * name, bool defaultValue, bool required ) const
{
    ASSERT( name );

    const BFFVariable * v = BFFStackFrame::GetVar( name );
    if ( v == nullptr )
    {
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
            return false;
        }
        var = defaultValue;
        return true;
    }

    if ( v->IsBool() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( iter, this, name, v->GetType(), BFFVariable::VAR_BOOL );
        return false;
    }

    var = v->GetBool();
    return true;
}

// GetInt
//------------------------------------------------------------------------------
bool Function::GetInt( const BFFIterator & iter, int32_t & var, const char * name, int32_t defaultValue, bool required ) const
{
    ASSERT( name );

    const BFFVariable * v = BFFStackFrame::GetVar( name );
    if ( v == nullptr )
    {
        if ( required )
        {
            Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
            return false;
        }
        var = defaultValue;
        return true;
    }

    if ( v->IsInt() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( iter, this, name, v->GetType(), BFFVariable::VAR_INT );
        return false;
    }

    var = v->GetInt();
    return true;
}


// GetInt
//------------------------------------------------------------------------------
bool Function::GetInt( const BFFIterator & iter, int32_t & var, const char * name, int32_t defaultValue, bool required, int minVal, int maxVal ) const
{
    if ( GetInt( iter, var, name, defaultValue, required ) == false )
    {
        return false;
    }

    // enforce additional limits
    if ( ( var < minVal ) || ( var > maxVal ) )
    {
        Error::Error_1054_IntegerOutOfRange( iter, this, name, minVal, maxVal );
        return false;
    }
    return true;
}

// GetNodeList
//------------------------------------------------------------------------------
bool Function::GetNodeList( NodeGraph & nodeGraph, const BFFIterator & iter, const char * propertyName, Dependencies & nodes, bool required,
                            bool allowCopyDirNodes, bool allowUnityNodes, bool allowRemoveDirNodes ) const
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

            if ( !GetNodeList( nodeGraph, iter, this, propertyName, nodeName, nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
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

        if ( !GetNodeList( nodeGraph, iter, this, propertyName, var->GetString(), nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
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
bool Function::GetDirectoryListNodeList( NodeGraph & nodeGraph,
                                         const BFFIterator & iter,
                                         const Array< AString > & paths,
                                         const Array< AString > & excludePaths,
                                         const Array< AString > & filesToExclude,
                                         const Array< AString > & excludePatterns,
                                         bool recurse,
                                         const Array< AString > * patterns,
                                         const char * inputVarName,
                                         Dependencies & nodes ) const
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
        DirectoryListNode::FormatName( path, patterns, recurse, excludePaths, filesToExcludeCleaned, excludePatterns, name );
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
            if ( !dln->Initialize( nodeGraph, iter, this ) )
            {
                return false; // Initialize will have emitted an error
            }
        }
        else if ( node->GetType() != Node::DIRECTORY_LIST_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, this, inputVarName, node->GetName(), node->GetType(), Node::DIRECTORY_LIST_NODE );
            return false;
        }

        nodes.Append( Dependency( node ) );
    }
    return true;
}

// GetFileNode
//------------------------------------------------------------------------------
bool Function::GetFileNode( NodeGraph & nodeGraph,
                            const BFFIterator & iter,
                            const AString & file,
                            const char * inputVarName,
                            Dependencies & nodes ) const
{
    // get node for the dir we depend on
    Node * node = nodeGraph.FindNode( file );
    if ( node == nullptr )
    {
        node = nodeGraph.CreateFileNode( file );
    }
    else if ( node->IsAFile() == false )
    {
        Error::Error_1005_UnsupportedNodeType( iter, this, inputVarName, node->GetName(), node->GetType() );
        return false;
    }

    nodes.Append( Dependency( node ) );
    return true;
}

// GetFileNodes
//------------------------------------------------------------------------------
bool Function::GetFileNodes( NodeGraph & nodeGraph,
                             const BFFIterator & iter,
                             const Array< AString > & files,
                             const char * inputVarName,
                             Dependencies & nodes ) const
{
    const AString * const  end = files.End();
    for ( const AString * it = files.Begin(); it != end; ++it )
    {
        const AString & file = *it;
        if (!GetFileNode( nodeGraph, iter, file, inputVarName, nodes ))
        {
            return false; // GetFileNode will have emitted an error
        }
    }
    return true;
}

// GetObjectListNodes
//------------------------------------------------------------------------------
bool Function::GetObjectListNodes( NodeGraph & nodeGraph,
                                   const BFFIterator & iter,
                                   const Array< AString > & objectLists,
                                   const char * inputVarName,
                                   Dependencies & nodes ) const
{
    const AString * const  end = objectLists.End();
    for ( const AString * it = objectLists.Begin(); it != end; ++it )
    {
        const AString & objectList = *it;

        // get node for the dir we depend on
        Node * node = nodeGraph.FindNode( objectList );
        if ( node == nullptr )
        {
            Error::Error_1104_TargetNotDefined( iter, this, inputVarName, objectList );
            return false;
        }
        else if ( node->GetType() != Node::OBJECT_LIST_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, this, inputVarName, node->GetName(), node->GetType(), Node::OBJECT_LIST_NODE );
            return false;
        }

        nodes.Append( Dependency( node ) );
    }
    return true;
}

// GetNodeList
//------------------------------------------------------------------------------
/*static*/ bool Function::GetNodeList( NodeGraph & nodeGraph,
                                       const BFFIterator & iter,
                                       const Function * function,
                                       const char * propertyName,
                                       const AString & nodeName,
                                       Dependencies & nodes,
                                       bool allowCopyDirNodes,
                                       bool allowUnityNodes,
                                       bool allowRemoveDirNodes )
{
    // get node
    Node * n = nodeGraph.FindNode( nodeName );
    if ( n == nullptr )
    {
        // not found - create a new file node
        n = nodeGraph.CreateFileNode( nodeName );
        nodes.Append( Dependency( n ) );
        return true;
    }

    // found - is it a file?
    if ( n->IsAFile() )
    {
        // found file - just use as is
        nodes.Append( Dependency( n ) );
        return true;
    }

    // found - is it an ObjectList?
    if ( n->GetType() == Node::OBJECT_LIST_NODE )
    {
        // use as-is
        nodes.Append( Dependency( n ) );
        return true;
    }

    // extra types
    if ( allowCopyDirNodes )
    {
        // found - is it an ObjectList?
        if ( n->GetType() == Node::COPY_DIR_NODE )
        {
            // use as-is
            nodes.Append( Dependency( n ) );
            return true;
        }
    }
    if ( allowRemoveDirNodes )
    {
        // found - is it a RemoveDirNode?
        if ( n->GetType() == Node::REMOVE_DIR_NODE )
        {
            // use as-is
            nodes.Append( Dependency( n ) );
            return true;
        }
    }
    if ( allowUnityNodes )
    {
        // found - is it an ObjectList?
        if ( n->GetType() == Node::UNITY_NODE )
        {
            // use as-is
            nodes.Append( Dependency( n ) );
            return true;
        }
    }

    // found - is it a group?
    if ( n->GetType() == Node::ALIAS_NODE )
    {
        AliasNode * an = n->CastTo< AliasNode >();
        const Dependencies & aNodes = an->GetAliasedNodes();
        for ( const Dependency * it = aNodes.Begin(); it != aNodes.End(); ++it )
        {
            // TODO:C by passing as string we'll be looking up again for no reason
            const AString & subName = it->GetNode()->GetName();

            if ( !GetNodeList( nodeGraph, iter, function, propertyName, subName, nodes, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
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
bool Function::GetStrings( const BFFIterator & iter, Array< AString > & strings, const char * name, bool required ) const
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

// GetFolderPaths
//------------------------------------------------------------------------------
bool Function::GetFolderPaths(const BFFIterator & iter, Array< AString > & paths, const char * name, bool required) const
{
    if ( !GetStrings(iter, paths, name, required ) )
    {
        return false; // GetStrings will have emitted an error
    }
    CleanFolderPaths( paths );
    return true;
}

// GetFileNode
//------------------------------------------------------------------------------
bool Function::GetFileNode( NodeGraph & nodeGraph, const BFFIterator & iter, Node * & fileNode, const char * name, bool required ) const
{
    // get the string containing the node name
    AStackString<> fileNodeName;
    if ( GetString( iter, fileNodeName, name, required ) == false )
    {
        return false;
    }

    // handle not-present
    if ( fileNodeName.IsEmpty() )
    {
        ASSERT( required == false ); // GetString should have managed required string
        fileNode = nullptr;
        return true;
    }

    // get/create the FileNode
    Node * n = nodeGraph.FindNode( fileNodeName );
    if ( n == nullptr )
    {
        n = nodeGraph.CreateFileNode( fileNodeName );
    }
    else if ( n->IsAFile() == false )
    {
        Error::Error_1103_NotAFile( iter, this, name, n->GetName(), n->GetType() );
        return false;
    }
    fileNode = n;
    return true;
}

// CleanFolderPaths
//------------------------------------------------------------------------------
/*static*/ void Function::CleanFolderPaths( Array< AString > & folders )
{
    AStackString< 512 > tmp;

    AString * const end = folders.End();
    for ( AString * it = folders.Begin(); it != end; ++it )
    {
        // make full path, clean slashes etc
        NodeGraph::CleanPath( *it, tmp );

        // ensure path is slash-terminated
        PathUtils::EnsureTrailingSlash( tmp );

        // replace original
        *it = tmp;
    }
}

//------------------------------------------------------------------------------
/*static*/ void Function::CleanFilePaths( Array< AString > & files )
{
    AStackString< 512 > tmp;

    AString * const end = files.End();
    for ( AString * it = files.Begin(); it != end; ++it )
    {
        // make full path, clean slashes etc
        NodeGraph::CleanPath( *it, tmp );

        // replace original
        *it = tmp;
    }
}

// CleanFileNames
//------------------------------------------------------------------------------
void Function::CleanFileNames( Array< AString > & fileNames ) const
{
    // cleanup slashes (keep path relative)
    AString * const end = fileNames.End();
    for ( AString * it = fileNames.Begin(); it != end; ++it )
    {
        // normalize slashes
        PathUtils::FixupFilePath( *it );
    }
}

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( NodeGraph & nodeGraph, const BFFIterator & iter, Node * nodeToAlias ) const
{
    Dependencies nodesToAlias( 1, false );
    nodesToAlias.Append( Dependency( nodeToAlias ) );
    return ProcessAlias( nodeGraph, iter, nodesToAlias );
}

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( NodeGraph & nodeGraph, const BFFIterator & iter, Dependencies & nodesToAlias ) const
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
bool Function::GetNameForNode( NodeGraph & nodeGraph, const BFFIterator & iter, const ReflectionInfo * ri, AString & name ) const
{
    // get object MetaData
    const Meta_Name * nameMD = ri->HasMetaData< Meta_Name >();
    ASSERT( nameMD ); // should not call this on types without this MetaData

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
        Array< AString > strings;
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
bool Function::PopulateProperties( NodeGraph & nodeGraph, const BFFIterator & iter, Node * node ) const
{
    const ReflectionInfo * ri = node->GetReflectionInfoV();
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

            if ( !PopulateProperty( nodeGraph, iter, node, property, v ) )
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
                                 const BFFIterator & iter,
                                 void * base,
                                 const ReflectedProperty & property,
                                 const BFFVariable * variable ) const
{
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
bool Function::PopulateStringHelper( NodeGraph & nodeGraph, const BFFIterator & iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, Array< AString > & outStrings ) const
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
                                     const BFFIterator & iter,
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
        Node * node = nodeGraph.FindNode( string );
        if ( node )
        {
            if ( node->GetType() == Node::ALIAS_NODE )
            {
                AliasNode * aliasNode = node->CastTo< AliasNode >();
                for ( const auto& aliasedNode : aliasNode->GetAliasedNodes() )
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
bool Function::PopulatePathAndFileHelper( const BFFIterator & iter,
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
bool Function::PopulateArrayOfStrings( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const
{
    Array< AString > strings;
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
bool Function::PopulateString( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const
{
    Array< AString > strings;
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
bool Function::PopulateBool( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
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
bool Function::PopulateInt32( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
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
bool Function::PopulateUInt32( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const
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
                                       const BFFIterator & iter,
                                       void * base,
                                       const ReflectedProperty & property,
                                       const BFFVariable * variable ) const
{
    // Get the destionation
    const ReflectedPropertyStruct & dstStructs = static_cast< const ReflectedPropertyStruct & >( property );
    ASSERT( dstStructs.IsArray() );
    const ReflectionInfo * ri = dstStructs.GetStructReflectionInfo();

    // Array to Array
    if ( variable->IsArrayOfStructs() )
    {
        // pre-size the destination
        const auto & srcStructs = variable->GetArrayOfStructs();
        dstStructs.ResizeArrayOfStruct( base, srcStructs.GetSize() );

        // Set the properties of each struct
        size_t index( 0 );
        for ( const auto * s : srcStructs )
        {
            // Calculate the base for this struct in the array
            void * structBase = dstStructs.GetStructInArray( base, index );

            // Try to populate all the properties for this struct
            for ( auto it = ri->Begin(); it != ri->End(); ++it )
            {
                AStackString<> propertyName( "." ); // TODO:C Eliminate copy
                propertyName += (*it).GetName();

                // Try to find property in BFF
                const BFFVariable ** found = BFFVariable::GetMemberByName( propertyName, s->GetStructMembers() );
                const BFFVariable * var = found ? *found : nullptr;
                if ( !PopulateProperty( nodeGraph, iter, structBase, *it, var ) )
                {
                    return false; // PopulateProperty will have emitted an error
                }
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

        // Try to populate all the properties for this struct
        for ( auto it = ri->Begin(); it != ri->End(); ++it )
        {
            AStackString<> propertyName( "." ); // TODO:C Eliminate copy
            propertyName += (*it).GetName();

            // Try to find property in BFF
            const BFFVariable ** found = BFFVariable::GetMemberByName( propertyName, variable->GetStructMembers() );
            const BFFVariable * var = found ? *found : nullptr;
            if ( !PopulateProperty( nodeGraph, iter, structBase, *it, var ) )
            {
                return false; // PopulateProperty will have emitted an error
            }
        }
        return true;
    }

    Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRUCT, BFFVariable::VAR_ARRAY_OF_STRUCTS );
    return false;
}

//------------------------------------------------------------------------------
