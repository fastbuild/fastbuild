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
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/MetaData/Meta_File.h"
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
                                         const Array< AString > & filesToExclude,
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
		DirectoryListNode::FormatName( path, pattern, recurse, excludePaths, filesToExclude, name );
		Node * node = ng.FindNode( name );
		if ( node == nullptr )
		{
			node = ng.CreateDirectoryListNode( name,
											   path,
											   pattern,
											   recurse,
											   excludePaths, 
                                               filesToExclude );
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

// GetFileNodes
//------------------------------------------------------------------------------
bool Function::GetFileNodes( const BFFIterator & iter,
                             const Array< AString > & files,
                             const char * inputVarName,
                             Dependencies & nodes ) const
{
    NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	const AString * const  end = files.End();
	for ( const AString * it = files.Begin(); it != end; ++it )
	{
		const AString & file = *it;

		// get node for the dir we depend on
		Node * node = ng.FindNode( file );
		if ( node == nullptr )
		{
			node = ng.CreateFileNode( file );
        }
		else if ( node->IsAFile() == false )
		{
			Error::Error_1005_UnsupportedNodeType( iter, this, inputVarName, node->GetName(), node->GetType() );
			return false;
		}

		nodes.Append( Dependency( node ) );
	}
	return true;
}

// GetObjectListNodes
//------------------------------------------------------------------------------
bool Function::GetObjectListNodes( const BFFIterator & iter,
                                   const Array< AString > & objectLists,
                                   const char * inputVarName,
                                   Dependencies & nodes ) const
{
    NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	const AString * const  end = objectLists.End();
	for ( const AString * it = objectLists.Begin(); it != end; ++it )
	{
		const AString & objectList = *it;

		// get node for the dir we depend on
		Node * node = ng.FindNode( objectList );
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
#ifdef USE_NODE_REFLECTION
    AliasNode * an = ng.CreateAliasNode( m_AliasForFunction );
    an->m_StaticDependencies = nodesToAlias; // TODO: make this use m_Targets & Initialize()
#else
	VERIFY( ng.CreateAliasNode( m_AliasForFunction, nodesToAlias ) );
#endif

	// clear the string so it can't be used again
	m_AliasForFunction.Clear();

	return true;
}

// GetNameForNode
//------------------------------------------------------------------------------
bool Function::GetNameForNode( const BFFIterator & iter, const ReflectionInfo * ri, AString & name ) const
{
	// get object MetaData
	const Meta_Name * nameMD = ri->HasMetaData< Meta_Name >();
	ASSERT( nameMD ); // should not call this on types without this MetaData

	// Format "Name" as ".Name" - TODO:C Would be good to eliminate this string copy
	AStackString<> propertyName( "." );
	propertyName += nameMD->GetName();

	// Find the value for this property from the BFF
	const BFFVariable * variable = BFFStackFrame::GetVar( propertyName );
	if ( variable->IsString() )
	{
		// Handle empty strings
		if ( variable->GetString().IsEmpty() )
		{
			Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, variable->GetName().Get() );
			return false;
		}

		AStackString<> string( variable->GetString() );

		// Path/File fixup needed?
		if ( !PopulatePathAndFileHelper( iter,
										 ri->HasMetaData< Meta_Path >(), 
										 ri->HasMetaData< Meta_File >(), 
										 variable->GetName(),
										 variable->GetString(),
										 string ) )
		{
			return false;
		}

		// Check that name isn't already used
		NodeGraph & ng = FBuild::Get().GetDependencyGraph();
		if ( ng.FindNode( string ) )
		{
			Error::Error_1100_AlreadyDefined( iter, this, string );
			return false;
		}

		name = string;
		return true;
	}

	Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING );
	return false;
}

// PopulateProperties
//------------------------------------------------------------------------------
bool Function::PopulateProperties( const BFFIterator & iter, Node * node ) const
{
	const ReflectionInfo * const ri = node->GetReflectionInfoV();
	const ReflectionIter end = ri->End();
	for ( ReflectionIter it = ri->Begin(); it != end; ++it )
	{
		const ReflectedProperty & property = *it;

		// Format "Name" as ".Name" - TODO:C Would be good to eliminate this string copy
		AStackString<> propertyName( "." );
		propertyName += property.GetName();

		// Find the value for this property from the BFF
		const BFFVariable * v = BFFStackFrame::GetVar( propertyName );

		// Handle missing but required
		if ( v == nullptr )
		{
			const bool required = ( property.HasMetaData< Meta_Optional >() == nullptr );
			if ( required )
			{
				Error::Error_1101_MissingProperty( iter, this, propertyName );
				return false;
			}

			continue; // missing but not required
		}

		const PropertyType pt = property.GetType();
		switch ( pt )
		{
			case PT_ASTRING:
			{
				if ( property.IsArray() )
				{
					if ( !PopulateArrayOfStrings( iter, node, property, v ) )
					{
						return false;
					}
				}
				else
				{
					if ( !PopulateString( iter, node, property, v ) )
					{
						return false;
					}
				}
				break;
			}
			case PT_BOOL:
			{
				if ( !PopulateBool( iter, node, property, v ) )
				{
					return false;
				}
				break;
			}
			case PT_UINT32:
			{
				if ( !PopulateUInt32( iter, node, property, v ) )
				{
					return false;
				}
				break;
			}
			default:
			{
				ASSERT( false ); // Unsupported type
				break;
			}
		}
	}
	return true;
}

// PopulatePathAndFileHelper
//------------------------------------------------------------------------------
bool Function::PopulatePathAndFileHelper( const BFFIterator & iter,
										  const Meta_Path * pathMD,
										  const Meta_File * fileMD,
										  const AString & variableName,
										  const AString & originalValue, 
										  AString & valueToFix ) const
{
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
			NodeGraph::CleanPath( originalValue, valueToFix );

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
			NodeGraph::CleanPath( originalValue, valueToFix );
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
bool Function::PopulateArrayOfStrings( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const
{
	// Array to Array
	if ( variable->IsArrayOfStrings() )
	{
		Array< AString > strings( variable->GetArrayOfStrings() );

		Array< AString >::Iter end = strings.End();
		for ( Array< AString >::Iter it = strings.Begin();
				it != end;
				++it )
		{
			AString & string = *it;

			// Path/File fixup needed?
			if ( !PopulatePathAndFileHelper( iter,
											 property.HasMetaData< Meta_Path >(), 
											 property.HasMetaData< Meta_File >(), 
											 variable->GetName(),
											 variable->GetArrayOfStrings()[ it - strings.Begin() ],
											 string ) )
			{
				return false;
			}
		}

		property.SetProperty( node, strings );
		return true;
	}

	if ( variable->IsString() )
	{
		// Handle empty strings
		if ( variable->GetString().IsEmpty() )
		{
			Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, variable->GetName().Get() );
			return false;
		}

		AStackString<> string( variable->GetString() );

		// Path/File fixup needed?
		if ( !PopulatePathAndFileHelper( iter, 
										 property.HasMetaData< Meta_Path >(), 
										 property.HasMetaData< Meta_File >(), 
										 variable->GetName(),
										 variable->GetString(),
										 string ) )
		{
			return false;
		}

		// Make array with 1 string
		Array< AString > strings( 1, false );
		strings.Append( string );
		property.SetProperty( node, strings );
		return true;
	}

	Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
	return false;
}

// PopulateString
//------------------------------------------------------------------------------
bool Function::PopulateString( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const
{
	if ( variable->IsString() )
	{
		// Handle empty strings
		if ( variable->GetString().IsEmpty() )
		{
			Error::Error_1004_EmptyStringPropertyNotAllowed( iter, this, variable->GetName().Get() );
			return false;
		}

		AStackString<> string( variable->GetString() );

		// Path/File fixup needed?
		if ( !PopulatePathAndFileHelper( iter,
											property.HasMetaData< Meta_Path >(), 
											property.HasMetaData< Meta_File >(), 
											variable->GetName(),
											variable->GetString(),
											string ) )
		{
			return false;
		}

		// String to String
		property.SetProperty( node, string );
		return true;
	}

	Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_STRING );
	return false;
}

// PopulateBool
//------------------------------------------------------------------------------
bool Function::PopulateBool( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const
{
	if ( variable->IsBool() )
	{
		// Bool to Bool
		property.SetProperty( node, variable->GetBool() );
		return true;
	}

	Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_BOOL );
	return false;
}

// PopulateUInt32
//------------------------------------------------------------------------------
bool Function::PopulateUInt32( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const
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
		property.SetProperty( node, (uint32_t)value );
		return true;
	}

	Error::Error_1050_PropertyMustBeOfType( iter, this, variable->GetName().Get(), variable->GetType(), BFFVariable::VAR_INT );
	return false;
}

//------------------------------------------------------------------------------
