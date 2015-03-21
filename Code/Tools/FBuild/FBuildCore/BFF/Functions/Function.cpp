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
#include "FunctionLibrary.h"
#include "FunctionObjectList.h"
#include "FunctionPrint.h"
#include "FunctionSettings.h"
#include "FunctionTest.h"
#include "FunctionUnity.h"
#include "FunctionUsing.h"
#include "FunctionVCXProject.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

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
Function::~Function()
{
}

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
	FNEW( FunctionLibrary );
	FNEW( FunctionPrint );
	FNEW( FunctionSettings );
	FNEW( FunctionTest );
	FNEW( FunctionUnity );
	FNEW( FunctionUsing );
	FNEW( FunctionVCXProject );
	FNEW( FunctionObjectList );
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
/*virtual*/ bool Function::ParseFunction( const BFFIterator & functionNameStart,
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
	BFFParser subParser;
	BFFIterator subIter( *functionBodyStartToken );
	subIter++; // skip past opening body token
	subIter.SetMax( functionBodyStopToken->GetCurrent() ); // cap parsing to body end
	if ( subParser.Parse( subIter ) == false )
	{
		return false;
	}

	// complete the function
	return Commit( functionNameStart );
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool Function::Commit( const BFFIterator & funcStartIter ) const
{
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
	if ( v->GetString().IsEmpty() )
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
	else
	{
		var.Clear();
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
	Array< AString > paths;
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
bool Function::GetNodeList( const BFFIterator & iter, const char * name, Dependencies & nodes, bool required,
							bool allowCopyDirNodes, bool allowUnityNodes ) const
{
	ASSERT( name );

	const BFFVariable * var = BFFStackFrame::GetVar( name );
	if ( !var )
	{
		// missing
		if ( required )
		{
			Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
			return false; // required!
		}
		return true; // missing but not required
	}

	if ( var->IsArrayOfStrings() )
	{
		// an array of references
		const Array< AString > & nodeNames = var->GetArrayOfStrings();
		nodes.SetCapacity( nodes.GetSize() + nodeNames.GetSize() );
		for ( Array< AString >::Iter it = nodeNames.Begin();
				it != nodeNames.End();
				it++ )
		{
			if ( it->IsEmpty() )
			{
				Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, name );
				return false;
			}

			if ( !GetNodeListRecurse( iter, name, nodes, *it, allowCopyDirNodes, allowUnityNodes ) )
			{
				// child func will have emitted error
				return false;
			}
		}
	}
	else if ( var->IsString() )
	{
		if ( var->GetString().IsEmpty() )
		{
			Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, name );
			return false;
		}

		if ( !GetNodeListRecurse( iter, name, nodes, var->GetString(), allowCopyDirNodes, allowUnityNodes ) )
		{
			// child func will have emitted error
			return false;
		}
	}
	else
	{
		// unsupported type
		Error::Error_1050_PropertyMustBeOfType( iter, this, name, var->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
		return false;
	}

	return true;
}

// GetDirectoryNodeList
//------------------------------------------------------------------------------
bool Function::GetDirectoryListNodeList( const BFFIterator & iter,
										 const Array< AString > & paths,
										 const Array< AString > & excludePaths,
										 bool recurse,
										 const AString & pattern,
										 const char * inputVarName,
										 Dependencies & nodes ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	const AString * const  end = paths.End();
	for ( const AString * it = paths.Begin(); it != end; ++it )
	{
		const AString & path = *it;

		// get node for the dir we depend on
		AStackString<> name;
		DirectoryListNode::FormatName( path, pattern, recurse, excludePaths, name );
		Node * node = ng.FindNode( name );
		if ( node == nullptr )
		{
			node = ng.CreateDirectoryListNode( name,
											   path,
											   pattern,
											   recurse,
											   excludePaths );
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

// GetNodeListRecurse
//------------------------------------------------------------------------------
bool Function::GetNodeListRecurse( const BFFIterator & iter, const char * name, Dependencies & nodes, const AString & nodeName,
								   bool allowCopyDirNodes, bool allowUnityNodes ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// get node
	Node * n = ng.FindNode( nodeName );
	if ( n == nullptr )
	{
		// not found - create a new file node
		n = ng.CreateFileNode( nodeName );
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

			if ( !GetNodeListRecurse( iter, name, nodes, subName, allowCopyDirNodes, allowUnityNodes ) )
			{
				return false;
			}
		}
		return true;
	}

	// don't know how to handle this type of node
	Error::Error_1005_UnsupportedNodeType( iter, this, name, n->GetName(), n->GetType() );
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
bool Function::GetFileNode( const BFFIterator & iter, Node * & fileNode, const char * name, bool required ) const
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
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * n = ng.FindNode( fileNodeName );
	if ( n == nullptr )
	{
		n = ng.CreateFileNode( fileNodeName );
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
void Function::CleanFolderPaths( Array< AString > & folders ) const
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
void Function::CleanFilePaths( Array< AString > & files ) const
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

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( const BFFIterator & iter, Node * nodeToAlias ) const
{
	Dependencies nodesToAlias( 1, false );
	nodesToAlias.Append( Dependency( nodeToAlias ) );
	return ProcessAlias( iter, nodesToAlias );
}

// ProcessAlias
//------------------------------------------------------------------------------
bool Function::ProcessAlias( const BFFIterator & iter, Dependencies & nodesToAlias ) const
{
	if ( m_AliasForFunction.IsEmpty() )
	{
		return true; // no alias required
	}

	// check for duplicates
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( iter, this, m_AliasForFunction );
		return false;
	}

	// create an alias against the node
	VERIFY( ng.CreateAliasNode( m_AliasForFunction, nodesToAlias ) );

	// clear the string so it can't be used again
	m_AliasForFunction.Clear();

	return true;
}

//------------------------------------------------------------------------------
