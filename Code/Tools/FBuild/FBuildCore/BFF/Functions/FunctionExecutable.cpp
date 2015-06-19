// FunctionExecutable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionExecutable.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionExecutable::FunctionExecutable()
: Function( "Executable" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExecutable::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExecutable::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	const BFFVariable * linker = BFFStackFrame::GetVar( ".Linker" );
	if ( linker == nullptr )
	{
		Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( "Linker" ) );
		return false;
	}
	AStackString<> linkerOutput;
	if ( !GetString( funcStartIter, linkerOutput, ".LinkerOutput", true ) )
	{
		return false; // GetString will have emitted error
	}
	if ( PathUtils::IsFolderPath( linkerOutput ) )
	{
		Error::Error_1105_PathNotAllowed( funcStartIter, this, ".LinkerOutput", linkerOutput );
		return false;
	}
	const BFFVariable * linkerOptions = BFFStackFrame::GetVar( ".LinkerOptions" );
	if ( linkerOptions == nullptr )
	{
		Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( "LinkerOptions" ) );
		return false;
	}
	const BFFVariable * libraries = BFFStackFrame::GetVar( ".Libraries" );
	if ( libraries == nullptr )
	{
		Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( "Libraries" ) );
		return false;
	}

	// Check input/output for Linker
	{
		const AString & args = linkerOptions->GetString();
		bool hasInputToken = ( args.Find( "%1" ) || args.Find( "\"%1\"" ) );
		if ( hasInputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".LinkerOptions", "%1" );
			return false;
		}
		bool hasOutputToken = ( args.Find( "%2" ) || args.Find( "\"%2\"" ) );
		if ( hasOutputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".LinkerOptions", "%2" );
			return false;
		}
	}

	// Optional linker stamping
	Node * linkerStampExe( nullptr );
	AStackString<> linkerStampExeArgs;
	if ( !GetFileNode( funcStartIter, linkerStampExe, ".LinkerStampExe" ) ||
		 !GetString( funcStartIter, linkerStampExeArgs, ".LinkerStampExeArgs" ) )
	{
		return false; // GetString will have emitted an error
	}

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// we'll build a list of libraries to link
	Dependencies libraryNodes( 64, true );

	// has a list been provided?
	if ( libraries->IsArrayOfStrings() )
	{
		// convert string list to nodes
		const Array< AString > & libraryNames = libraries->GetArrayOfStrings();
		for ( Array< AString >::ConstIter it = libraryNames.Begin();
			  it != libraryNames.End();
			  it++ )
		{
			if ( DependOnNode( funcStartIter, *it, libraryNodes ) == false )
			{
				return false; // DependOnNode will have emitted an error
			}
		}
	}
	else if ( libraries->IsString() )
	{
		// handle this one node
		if ( DependOnNode( funcStartIter, libraries->GetString(), libraryNodes ) == false )
		{
			return false; // DependOnNode will have emitted an error
		}
	}
	else
	{
		Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, "Libraries", libraries->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_ARRAY_OF_STRINGS );
		return false;
	}

	// Check for existing node
	if ( ng.FindNode( linkerOutput ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, linkerOutput );
		return false;
	}

	// Assembly Resources
	Dependencies assemblyResources;
	if ( !GetNodeList( funcStartIter, ".LinkerAssemblyResources", assemblyResources, false ) )
	{
		return false; // GetNodeList will have emitted error
	}

	// Determine flags
	uint32_t flags = LinkerNode::DetermineFlags( linker->GetString(), linkerOptions->GetString() );
	bool isADLL = ( ( flags & LinkerNode::LINK_FLAG_DLL ) != 0 );

	bool linkObjects = isADLL ? true : false;
	if ( GetBool( funcStartIter, linkObjects, ".LinkerLinkObjects", linkObjects, false ) == false )
	{
		return false;
	}
	if ( linkObjects )
	{
		flags |= LinkerNode::LINK_OBJECTS;
	}

	// get inputs not passed through 'LibraryNodes' (i.e. directly specified on the cmd line)
	Dependencies otherLibraryNodes( 64, true );
	if ( ( flags & ( LinkerNode::LINK_FLAG_MSVC | LinkerNode::LINK_FLAG_GCC | LinkerNode::LINK_FLAG_SNC | LinkerNode::LINK_FLAG_ORBIS_LD | LinkerNode::LINK_FLAG_GREENHILLS_ELXR | LinkerNode::LINK_FLAG_CODEWARRIOR_LD ) ) != 0 )
	{
		const bool msvcStyle = ( ( flags & LinkerNode::LINK_FLAG_MSVC ) == LinkerNode::LINK_FLAG_MSVC );
		if ( !GetOtherLibraries( funcStartIter, linkerOptions->GetString(), otherLibraryNodes, msvcStyle ) )
		{
			return false; // will have emitted error
		}
	}

	// check for Import Library override
	AStackString<> importLibName;
	if ( isADLL && ( ( flags & LinkerNode::LINK_FLAG_MSVC ) != 0 )  )
	{
		GetImportLibName( linkerOptions->GetString(), importLibName );
	}

	// make node for exe
	Node * n( nullptr );
	if ( isADLL )
	{
		n = ng.CreateDLLNode( linkerOutput,
							  libraryNodes,
							  otherLibraryNodes,
							  linker->GetString(),
							  linkerOptions->GetString(),
							  flags,
							  assemblyResources,
							  importLibName,
							  linkerStampExe,
							  linkerStampExeArgs );
	}
	else
	{
		n = ng.CreateExeNode( linkerOutput,
							  libraryNodes,
							  otherLibraryNodes,
							  linker->GetString(),
							  linkerOptions->GetString(),
							  flags,
							  assemblyResources,
							  linkerStampExe,
							  linkerStampExeArgs );
	}

	return ProcessAlias( funcStartIter, n );
}

// DependOnNode
//------------------------------------------------------------------------------
bool FunctionExecutable::DependOnNode( const BFFIterator & iter, const AString & nodeName, Dependencies & nodes ) const
{
	// silently ignore empty nodes
	if ( nodeName.IsEmpty() )
	{
		return true;
	}

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * node = ng.FindNode( nodeName );

	// does it exist?
	if ( node != nullptr )
	{
		// process it
		return DependOnNode( iter, node, nodes );
	}

	// node not found - create a new FileNode, assuming we are
	// linking against an externally built library
	node = ng.CreateFileNode( nodeName );
	nodes.Append( Dependency( node ) );
	return true;
}

// DependOnNode
//------------------------------------------------------------------------------
bool FunctionExecutable::DependOnNode( const BFFIterator & iter, Node * node, Dependencies & nodes ) const
{
	ASSERT( node );

	// a previously declared library?
	if ( node->GetType() == Node::LIBRARY_NODE )
	{
		// can link directly to it
		nodes.Append( Dependency( node ) );
		return true;
	}

	// a previously declared object list?
	if ( node->GetType() == Node::OBJECT_LIST_NODE )
	{
		// can link directly to it
		nodes.Append( Dependency( node ) );
		return true;
	}

	// a dll?
	if ( node->GetType() == Node::DLL_NODE )
	{
		// TODO:B Depend on import lib
		nodes.Append( Dependency( node, true ) ); // NOTE: Weak dependency
		return true;
	}

	// a previously declared external file?
	if ( node->GetType() == Node::FILE_NODE )
	{
		// can link directy against it
		nodes.Append( Dependency( node ) );
		return true;
	}

	// a file copy?
	if ( node->GetType() == Node::COPY_NODE )
	{
		// depend on copy - will use input at build time
		nodes.Append( Dependency( node ) );
		return true;
	}

	// an external executable?
	if ( node->GetType() == Node::EXEC_NODE )
	{
		// depend on ndoe - will use exe output at build time
		nodes.Append( Dependency( node ) );
		return true;
	}

	// a group (alias)?
	if ( node->GetType() == Node::ALIAS_NODE )
	{
		// handle all targets in alias
		AliasNode * an = node->CastTo< AliasNode >();
		const Dependencies & aliasNodeList = an->GetAliasedNodes();
		const Dependencies::Iter end = aliasNodeList.End();
		for ( Dependencies::Iter it = aliasNodeList.Begin();
			  it != end; 
			  ++it )
		{
			if ( DependOnNode( iter, it->GetNode(), nodes ) == false )
			{
				return false; // something went wrong lower down
			}
		}
		return true; // all nodes in group handled ok
	}

	// don't know how to handle this type of node
	Error::Error_1005_UnsupportedNodeType( iter, this, "Libraries", node->GetName(), node->GetType() );
	return false;
}

// GetImportLibName
//------------------------------------------------------------------------------
void FunctionExecutable::GetImportLibName( const AString & args, AString & importLibName ) const
{
	// split to individual tokens
	Array< AString > tokens;
	args.Tokenize( tokens );

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it != end; ++it )
	{
		if ( it->BeginsWith( "/IMPLIB:") )
		{
			const char * impStart = it->Get() + 8;
			const char * impEnd = it->GetEnd();

			// if token is exactly /IMPLIB: then value is next token
			if ( impStart == impEnd )
			{
				++it;
				// handle missing next value
				if ( it == end )
				{
					return; // we just pretend it doesn't exist and let the linker complain
				}

				impStart = it->Get();
				impEnd = it->GetEnd();
			}

			Args::StripQuotes( impStart, impEnd, importLibName );
		}
	}
}

// GetOtherLibraries
//------------------------------------------------------------------------------
bool FunctionExecutable::GetOtherLibraries( const BFFIterator & iter, 
											const AString & args, 
											Dependencies & otherLibraries,
											bool msvc ) const
{
	// split to individual tokens
	Array< AString > tokens;
	args.Tokenize( tokens );

	bool ignoreAllDefaultLibs = false;
	Array< AString > defaultLibsToIgnore( 8, true );
	Array< AString > defaultLibs( 16, true );
	Array< AString > libs( 16, true );
	Array< AString > dashlLibs( 16, true );
	Array< AString > libPaths( 16, true );
	Array< AString > envLibPaths( 32, true );

	// extract lib path from system if present
	AStackString< 1024 > libVar;
	if ( Env::GetEnvVariable( "LIB", libVar ) )
	{
		libVar.Tokenize( envLibPaths, ';' );
	}

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it != end; ++it )
	{
		const AString & token = *it;

		// MSVC style
		if ( msvc )
		{
			// /NODEFAULTLIB
			if ( token == "/NODEFAULTLIB" )
			{
				ignoreAllDefaultLibs = true;
				continue;
			}

			// /NODEFAULTLIB:
			if ( GetOtherLibsArg( "/NODEFAULTLIB:", defaultLibsToIgnore, it, end ) )
			{
				continue;
			}

			// /DEFAULTLIB:
			if ( GetOtherLibsArg( "/DEFAULTLIB:", defaultLibs, it, end ) )
			{
				continue;
			}

			// /LIBPATH:
			if ( GetOtherLibsArg( "/LIBPATH:", libPaths, it, end, true ) ) // true = canonicalize path
			{
				continue;
			}

			// some other linker argument?
			if ( token.BeginsWith( '/' ) )
			{
				continue;
			}
		}

		// GCC/SNC style
		if ( !msvc )
		{
			// We don't need to check for this, as there is no default lib passing on
			// the cmd line.
			// -nodefaultlibs
			//if ( token == "-nodefaultlibs" )
			//{
			//	ignoreAllDefaultLibs = true;
			//	continue;
			//}

			// -L (lib path)
			if ( GetOtherLibsArg( "-L", libPaths, it, end ) )
			{
				continue;
			}

			// -l (lib)
			if ( GetOtherLibsArg( "-l", dashlLibs, it, end ) )
			{
				continue;
			}

			// some other linker argument?
			if ( token.BeginsWith( '-' ) )
			{
				continue;
			}
		}

		// build time substitution?
		if ( token.BeginsWith( '%' ) ||		// %1
			 token.BeginsWith( "'%" ) ||	// '%1'
			 token.BeginsWith( "\"%" ) )	// "%1"
		{
			continue;
		}

		// anything left is an input to the linker
		AStackString<> libName;
		Args::StripQuotes( token.Get(), token.GetEnd(), libName );
		if ( token.IsEmpty() == false )
		{
			libs.Append( libName );
		}
	}

	// filter default libs
	if ( ignoreAllDefaultLibs )
	{
		// filter all default libs
		defaultLibs.Clear();
	}
	else
	{
		// filter specifically listed default libs
		const AString * const endI = defaultLibsToIgnore.End();
		for ( const AString * itI = defaultLibsToIgnore.Begin(); itI != endI; ++itI )
		{
			const AString * const endD = defaultLibs.End();
			for ( AString * itD = defaultLibs.Begin(); itD != endD; ++itD )
			{
				if ( itI->CompareI( *itD ) == 0 )
				{
					defaultLibs.Erase( itD );
					break;
				}
			}
		}
	}

	if ( !msvc )
	{
		// convert -l<name> style libs to lib<name>.a
		const AString * const endDL = dashlLibs.End();
		for ( const AString * itDL = dashlLibs.Begin(); itDL != endDL; ++itDL )
		{
			AStackString<> libName;
			libName += "lib";
			libName += *itDL;
			libName += ".a";
			libs.Append( libName );
		}
	}

	// any remaining default libs are treated the same as libs
	libs.Append( defaultLibs );

	// use Environment libpaths if found (but used after LIBPATH provided ones)
	libPaths.Append( envLibPaths );

	// convert libs to nodes
	const AString * const endL = libs.End();
	for ( const AString * itL = libs.Begin(); itL != endL; ++itL )
	{
		bool found = false;

		// is the file a full path?
		if ( ( itL->GetLength() > 2 ) && ( (*itL)[ 1 ] == ':' ) )
		{
			// check file exists in current location
			if ( !GetOtherLibrary( iter, otherLibraries, AString::GetEmpty(), *itL, found ) )
			{
				return false; // GetOtherLibrary will have emitted error
			}
		}
		else
		{
			// check each libpath
			const AString * const endP = libPaths.End();
			for ( const AString * itP = libPaths.Begin(); itP != endP; ++itP )
			{
				if ( !GetOtherLibrary( iter, otherLibraries, *itP, *itL, found ) )
				{
					return false; // GetOtherLibrary will have emitted error
				}

				if ( found )
				{
					break;
				}

				// keep searching lib paths...
			}
		}

		// file does not exist on disk, and there is no rule to build it
        // Don't complain about this, because:
        //  a) We may be parsing rules on another OS (i.e. parsing Linux rules on Windows)
        //  b) User may have filtered some libs for platforms they don't care about (i.e. libs
        //     for PS4 on a PC developer's machine on a cross-platform team)
        // If the file is actually needed, the linker will emit an error during link-time.
	}

	return true;
}

// GetOtherLibrary
//------------------------------------------------------------------------------
bool FunctionExecutable::GetOtherLibrary( const BFFIterator & iter, Dependencies & libs, const AString & path, const AString & lib, bool & found ) const
{
	found = false;

	AStackString<> potentialNodeName( path );
	if ( !potentialNodeName.IsEmpty() )
	{
		PathUtils::EnsureTrailingSlash( potentialNodeName );
	}
	potentialNodeName += lib;
	AStackString<> potentialNodeNameClean;
	NodeGraph::CleanPath( potentialNodeName, potentialNodeNameClean );

	// see if a node already exists
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * node = ng.FindNode( potentialNodeNameClean );
	if ( node )
	{
		// aliases not supported - must point to something that provides a file
		if ( node->IsAFile() == false )
		{
			Error::Error_1103_NotAFile( iter, this, ".LinkerOptions", potentialNodeNameClean, node->GetType() );
			return false;
		}

		// found existing node
		libs.Append( Dependency( node ) );
		found = true;
		return true; // no error
	}

	// see if the file exists on disk at this location
	if ( FileIO::FileExists( potentialNodeNameClean.Get() ) )
	{
		node = ng.CreateFileNode( potentialNodeNameClean );
		libs.Append( Dependency( node ) );
		found = true;
		FLOG_INFO( "Additional library '%s' assumed to be '%s'\n", lib.Get(), potentialNodeNameClean.Get() );
		return true; // no error
	}

	return true; // no error
}

// GetOtherLibsArg
//------------------------------------------------------------------------------
/*static*/ bool FunctionExecutable::GetOtherLibsArg( const char * arg, 
													 Array< AString > & list, 
													 const AString * & it, 
													 const AString * const & end,
													 bool canonicalizePath )
{
	// check for expected arg
	if ( it->BeginsWith( arg ) == false )
	{
		return false; // not our arg, not consumed
	}

	// get remainder of token after arg
	const char * valueStart = it->Get() + AString::StrLen( arg );
	const char * valueEnd = it->GetEnd();

	// if no remainder, arg value is next token
	if ( valueStart == valueEnd )
	{
		++it;

		// no more tokens? (malformed input)
		if ( it == end )
		{
			// ignore this item and let the linker complain about that
			return true; // arg consumed
		}

		// use next token a value
		valueStart = it->Get();
		valueEnd = it->GetEnd();
	}

	// eliminate quotes
	AStackString<> value;
	Args::StripQuotes( valueStart, valueEnd, value );

	// store if useful
	if ( value.IsEmpty() == false )
	{
		if ( canonicalizePath )
		{
			AStackString<> cleanValue;
			NodeGraph::CleanPath( value, cleanValue );
			PathUtils::EnsureTrailingSlash( cleanValue );
			list.Append( cleanValue );
		}
		else
		{
			list.Append( value );
		}
	}

	return true; // arg consumed
}

//------------------------------------------------------------------------------
