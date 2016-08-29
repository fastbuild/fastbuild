// FunctionObjectList
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionObjectList::FunctionObjectList()
: Function( "ObjectList" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::NeedsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	const BFFVariable * compiler;
	const BFFVariable * compilerOptions;
	AStackString<> compilerOptionsDeoptimized;
	AStackString<> compilerOutputPath;
    AStackString<> compilerOutputPrefix;
	const BFFVariable * compilerOutputExtension;
	if ( !GetString( funcStartIter, compiler, ".Compiler", true ) ||
		 !GetString( funcStartIter, compilerOptions, ".CompilerOptions", true ) ||
		 !GetString( funcStartIter, compilerOptionsDeoptimized, ".CompilerOptionsDeoptimized", false ) ||
		 !GetString( funcStartIter, compilerOutputPath, ".CompilerOutputPath", true ) ||
		 !GetString( funcStartIter, compilerOutputPrefix, ".CompilerOutputPrefix", false ) ||
		 !GetString( funcStartIter, compilerOutputExtension, ".CompilerOutputExtension", false ) )
	{
		return false;
	}

    PathUtils::FixupFolderPath( compilerOutputPath );

	// find or create the compiler node
	CompilerNode * compilerNode = nullptr;
	if ( !FunctionObjectList::GetCompilerNode( nodeGraph, funcStartIter, compiler->GetString(), compilerNode ) )
	{
		return false; // GetCompilerNode will have emitted error
	}

	// Compiler Force Using
	Dependencies compilerForceUsing;
	if ( !GetNodeList( nodeGraph, funcStartIter, ".CompilerForceUsing", compilerForceUsing, false ) )
	{
		return false; // GetNodeList will have emitted an error
	}	
	
	// de-optimization setting
	bool deoptimizeWritableFiles = false;
	bool deoptimizeWritableFilesWithToken = false;
	if ( !GetBool( funcStartIter, deoptimizeWritableFiles, ".DeoptimizeWritableFiles", false, false ) )
	{
		return false; // GetBool will have emitted error
	}
	if ( !GetBool( funcStartIter, deoptimizeWritableFilesWithToken, ".DeoptimizeWritableFilesWithToken", false, false ) )
	{
		return false; // GetBool will have emitted error
	}
	if ( ( deoptimizeWritableFiles || deoptimizeWritableFilesWithToken ) && compilerOptionsDeoptimized.IsEmpty() )
	{
		Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".CompilerOptionsDeoptimized" ) );
		return false;
	}

	// cache & distribution control
	bool allowDistribution( true );
	bool allowCaching( true );
	if ( !GetBool( funcStartIter, allowDistribution, ".AllowDistribution", true ) ||
		 !GetBool( funcStartIter, allowCaching, ".AllowCaching", true ) )
	{
		return false; // GetBool will have emitted error
	}

	// Precompiled Header support
	ObjectNode * precompiledHeaderNode = nullptr;
	AStackString<> compilerOutputExtensionStr( compilerOutputExtension ? compilerOutputExtension->GetString().Get() : ".obj" );
	if ( !GetPrecompiledHeaderNode( nodeGraph, funcStartIter, compilerNode, compilerOptions, compilerForceUsing, precompiledHeaderNode, deoptimizeWritableFiles, deoptimizeWritableFilesWithToken, allowDistribution, allowCaching, compilerOutputExtensionStr ) )
	{
		return false; // GetPrecompiledHeaderNode will have emitted error
	}	

	// Sanity check compile flags
	const bool usingPCH = ( precompiledHeaderNode != nullptr );
	uint32_t objFlags = ObjectNode::DetermineFlags( compilerNode, compilerOptions->GetString(), false, usingPCH );
	if ( ( objFlags & ObjectNode::FLAG_MSVC ) && ( objFlags & ObjectNode::FLAG_CREATING_PCH ) )
	{
		// must not specify use of precompiled header (must use the PCH specific options)
		Error::Error_1303_PCHCreateOptionOnlyAllowedOnPCH( funcStartIter, this, "/Yc", "CompilerOptions" );
		return false;
	}

	// Check input/output for Compiler
	{
		const AString & args = compilerOptions->GetString();
		bool hasInputToken = ( args.Find( "%1" ) || args.Find( "\"%1\"" ) );
		if ( hasInputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".CompilerOptions", "%1" );
			return false;
		}
		bool hasOutputToken = ( args.Find( "%2" ) || args.Find( "\"%2\"" ) );
		if ( hasOutputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".CompilerOptions", "%2" );
			return false;
		}

        // check /c or -c
        if ( objFlags & ObjectNode::FLAG_MSVC )
        {
            if ( args.Find( "/c" ) == nullptr &&
				args.Find( "-c" ) == nullptr)
            {
		        Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".CompilerOptions", "/c or -c" );
			    return false;
		    }
        }
        else if ( objFlags & ( ObjectNode::FLAG_SNC | ObjectNode::FLAG_GCC | ObjectNode::FLAG_CLANG ) )
        {
            if ( args.Find( "-c" ) == nullptr )
            {
		        Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".CompilerOptions", "-c" );
			    return false;
		    }
        }
	}

	// Get the (optional) Preprocessor & PreprocessorOptions
	const BFFVariable * preprocessor = nullptr;
	const BFFVariable * preprocessorOptions = nullptr;
    CompilerNode * preprocessorNode = nullptr;
	if ( !GetString( funcStartIter, preprocessor, ".Preprocessor", false ) )
	{
		return false; // GetString will have emitted an error
	}
	if ( preprocessor )
    {
		// get the preprocessor executable
        if ( !FunctionObjectList::GetCompilerNode( nodeGraph, funcStartIter, preprocessor->GetString(), preprocessorNode ) )
        {
            return false; // GetCompilerNode will have emitted an error
        }

		// get the command line args for the preprocessor
        if ( !GetString( funcStartIter, preprocessorOptions, ".PreprocessorOptions", true ) ) // required
		{
			return false; // GetString will have emitted an error
		}
    }

	// Pre-build dependencies
	Dependencies preBuildDependencies;
	if ( !GetNodeList( nodeGraph, funcStartIter, ".PreBuildDependencies", preBuildDependencies, false ) )
	{
		return false; // GetNodeList will have emitted an error
	}

	Dependencies staticDeps( 32, true );
	if ( !GetInputs( nodeGraph, funcStartIter, staticDeps ) )
	{
		return false; // GetStaticDeps will gave emitted error
	}

	if ( staticDeps.IsEmpty() )
	{
		Error::Error_1006_NothingToBuild( funcStartIter, this );
		return false;
	}

	// parsing logic should guarantee we have a string for our name
	ASSERT( m_AliasForFunction.IsEmpty() == false );

	// Check for existing node
	if ( nodeGraph.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	AStackString<> baseDirectory;
	if ( !GetBaseDirectory( funcStartIter, baseDirectory ) )
	{
		return false; // GetBaseDirectory will have emitted error
	}

	AStackString<> extraPDBPath, extraASMPath;
	GetExtraOutputPaths( compilerOptions->GetString(), extraPDBPath, extraASMPath );

	// Create library node which depends on the single file or list
	ObjectListNode * o = nodeGraph.CreateObjectListNode( m_AliasForFunction,
												  staticDeps,
												  compilerNode,
												  compilerOptions->GetString(),
												  compilerOptionsDeoptimized,
												  compilerOutputPath,
												  precompiledHeaderNode,
												  compilerForceUsing,
												  preBuildDependencies,
												  deoptimizeWritableFiles,
												  deoptimizeWritableFilesWithToken,
												  allowDistribution,
												  allowCaching,
                                                  preprocessorNode,
                                                  preprocessorOptions ? preprocessorOptions->GetString() : AString::GetEmpty(),
												  baseDirectory );
	if ( compilerOutputExtension )
	{
		o->m_ObjExtensionOverride = compilerOutputExtension->GetString();
	}
    o->m_CompilerOutputPrefix = compilerOutputPrefix;
	o->m_ExtraPDBPath = extraPDBPath;
	o->m_ExtraASMPath = extraASMPath;

	return true;
}

// GetCompilerNode
//------------------------------------------------------------------------------
bool FunctionObjectList::GetCompilerNode( NodeGraph & nodeGraph, const BFFIterator & iter, const AString & compiler, CompilerNode * & compilerNode ) const
{
	Node * cn = nodeGraph.FindNode( compiler );
	compilerNode = nullptr;
	if ( cn != nullptr )
	{
		if ( cn->GetType() == Node::ALIAS_NODE )
		{
			AliasNode * an = cn->CastTo< AliasNode >();
			cn = an->GetAliasedNodes()[ 0 ].GetNode();
		}
		if ( cn->GetType() != Node::COMPILER_NODE )
		{
			Error::Error_1102_UnexpectedType( iter, this, "Compiler", cn->GetName(), cn->GetType(), Node::COMPILER_NODE );
			return false;
		}
		compilerNode = cn->CastTo< CompilerNode >();
	}
	else
	{
		// create a compiler node - don't allow distribution
		// (only explicitly defined compiler nodes can be distributed)
		AStackString<> compilerClean;
		NodeGraph::CleanPath( compiler, compilerClean );
		compilerNode = nodeGraph.CreateCompilerNode( compilerClean );
        VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "AllowDistribution", false ) );
	}

	return true;
}

// GetPrecompiledHeaderNode
//------------------------------------------------------------------------------
bool FunctionObjectList::GetPrecompiledHeaderNode( NodeGraph & nodeGraph,
												   const BFFIterator & iter,
												   CompilerNode * compilerNode,
												   const BFFVariable * compilerOptions,
												   const Dependencies & compilerForceUsing,
												   ObjectNode * & precompiledHeaderNode,
												   bool deoptimizeWritableFiles,
												   bool deoptimizeWritableFilesWithToken,
												   bool allowDistribution,
												   bool allowCaching,
												   const AString& compilerOutputExtension ) const
{
	const BFFVariable * pchInputFile = nullptr;
	const BFFVariable * pchOutputFile = nullptr;
	const BFFVariable * pchOptions = nullptr;
	if ( !GetString( iter, pchInputFile, ".PCHInputFile" ) ||
		 !GetString( iter, pchOutputFile, ".PCHOutputFile" ) ||
		 !GetString( iter, pchOptions, ".PCHOptions" ) )
	{
		return false;
	}

	precompiledHeaderNode = nullptr;

	AStackString<> pchObjectName;

	if ( pchInputFile ) 
	{
		if ( !pchOutputFile || !pchOptions )
		{
			Error::Error_1300_MissingPCHArgs( iter, this );
			return false;
		}

		Node * pchInputNode = nodeGraph.FindNode( pchInputFile->GetString() );
		if ( pchInputNode )
		{
			// is it a file?
			if ( pchInputNode->IsAFile() == false )
			{
				Error::Error_1103_NotAFile( iter, this, "PCHInputFile", pchInputNode->GetName(), pchInputNode->GetType() );
				return false;
			}
		}
		else
		{
			// Create input node
			pchInputNode = nodeGraph.CreateFileNode( pchInputFile->GetString() );
		}

		if ( nodeGraph.FindNode( pchOutputFile->GetString() ) )
		{
			Error::Error_1301_AlreadyDefinedPCH( iter, this, pchOutputFile->GetString().Get() );
			return false;
		}

		uint32_t pchFlags = ObjectNode::DetermineFlags( compilerNode, pchOptions->GetString(), true, false );
		if ( pchFlags & ObjectNode::FLAG_MSVC )
		{
			// sanity check arguments

			// Find /Fo option to obtain pch object file name
			Array< AString > tokens;
			pchOptions->GetString().Tokenize( tokens );
			for ( const AString & token : tokens )
			{
				if ( token.BeginsWithI( "/Fo" ) )
				{
					// Extract filename (and remove quotes if found)
					pchObjectName = token.Get() + 3;
					pchObjectName.Trim( pchObjectName.BeginsWith( '"' ) ? 1 : 0, pchObjectName.EndsWith( '"' ) ? 1 : 0 );

					// Auto-generate name?
					if ( pchObjectName == "%3" )
					{
						// example 'PrecompiledHeader.pch' to 'PrecompiledHeader.pch.obj'
						pchObjectName = pchOutputFile->GetString();
						pchObjectName += compilerOutputExtension;
					}
					break;
				}
			}

			// PCH must have "Create PCH" (e.g. /Yc"PrecompiledHeader.h")
			if ( pchOptions->GetString().Find( "/Yc" ) == nullptr )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Yc", "PCHOptions" );
				return false;
			}
			// PCH must have "Precompiled Header to Use" (e.g. /Fp"PrecompiledHeader.pch")
			if ( pchOptions->GetString().Find( "/Fp" ) == nullptr )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Fp", "PCHOptions" );
				return false;
			}
			// PCH must have object output option (e.g. /Fo"PrecompiledHeader.obj")
			if ( pchObjectName.IsEmpty() )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Fo", "PCHOptions" );
				return false;
			}

			// Object using the PCH must have "Use PCH" option (e.g. /Yu"PrecompiledHeader.h")
			if ( compilerOptions->GetString().Find( "/Yu" ) == nullptr )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Yu", "CompilerOptions" );
				return false;
			}
			// Object using the PCH must have "Precompiled header to use" (e.g. /Fp"PrecompiledHeader.pch")
			if ( compilerOptions->GetString().Find( "/Fp" ) == nullptr )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Fp", "CompilerOptions" );
				return false;
			}
		}

		precompiledHeaderNode = nodeGraph.CreateObjectNode( pchOutputFile->GetString(),
													 pchInputNode,
													 compilerNode,
													 pchOptions->GetString(),
													 AString::GetEmpty(),
													 nullptr,
													 pchFlags,
													 compilerForceUsing,
													 deoptimizeWritableFiles,
													 deoptimizeWritableFilesWithToken,
												     allowDistribution,
													 allowCaching,
                                                     nullptr, AString::GetEmpty(), 0 ); // preprocessor args not supported
		precompiledHeaderNode->m_PCHObjectFileName = pchObjectName;
	}

	return true;
}

// GetInputs
//------------------------------------------------------------------------------
bool FunctionObjectList::GetInputs( NodeGraph & nodeGraph, const BFFIterator & iter, Dependencies & inputs ) const
{
	// do we want to build files via a unity blob?
    Array< AString > inputUnities;
    if ( !GetStrings( iter, inputUnities, ".CompilerInputUnity", false ) ) // not required
	{
		return false;
	}
    for ( const auto& unity : inputUnities )
    {
        Node * n = nodeGraph.FindNode( unity );
		if ( n == nullptr )
		{
			Error::Error_1104_TargetNotDefined( iter, this, "CompilerInputUnity", unity );
			return false;
		}
		if ( n->GetType() != Node::UNITY_NODE )
		{
			Error::Error_1102_UnexpectedType( iter, this, "CompilerInputUnity", unity, n->GetType(), Node::UNITY_NODE );
			return false;
		}
		inputs.Append( Dependency( n ) );
    }

	// do we want to build a files in a directory?
	const BFFVariable * inputPath = BFFStackFrame::GetVar( ".CompilerInputPath" );
	if ( inputPath )
	{
		// get the optional pattern and recurse options related to InputPath
		Array< AString > patterns;
		if ( !GetStrings( iter, patterns, ".CompilerInputPattern", false ) )
		{
			return false; // GetString will have emitted an error
		}
		if ( patterns.IsEmpty() )
		{
			patterns.Append( AStackString<>( "*.cpp" ) );
		}

		// recursive?  default to true
		bool recurse = true;
		if ( !GetBool( iter, recurse, ".CompilerInputPathRecurse", true, false ) )
		{
			return false; // GetBool will have emitted an error
		}

		// Support an exclusion path
		Array< AString > excludePaths;
		if ( !GetFolderPaths( iter, excludePaths, ".CompilerInputExcludePath", false ) )
		{
			return false; // GetFolderPaths will have emitted an error
		}

        Array< AString > filesToExclude;
        if ( !GetStrings( iter, filesToExclude, ".CompilerInputExcludedFiles", false ) ) // not required
        {
            return false; // GetStrings will have emitted an error
        }
	    CleanFileNames( filesToExclude );

		// Input paths
		Array< AString > inputPaths;
		if ( !GetFolderPaths( iter, inputPaths, ".CompilerInputPath", false ) )
		{
			return false; // GetFolderPaths will have emitted an error
		}

		Dependencies dirNodes( inputPaths.GetSize() );
		if ( !GetDirectoryListNodeList( nodeGraph, iter, inputPaths, excludePaths, filesToExclude, recurse, &patterns, "CompilerInputPath", dirNodes ) )
		{
			return false; // GetDirectoryListNodeList will have emitted an error
		}
		inputs.Append( dirNodes );
	}

	// do we want to build a specific list of files?
	if ( !GetNodeList( nodeGraph, iter, ".CompilerInputFiles", inputs, false ) )
	{
		// helper will emit error
		return false;
	}

	return true;
}

// GetBaseDirectory
//------------------------------------------------------------------------------
bool FunctionObjectList::GetBaseDirectory( const BFFIterator & iter, AStackString<> & baseDirectory) const
{
	AStackString<> baseDir;
	if ( !GetString( iter, baseDir, ".CompilerInputFilesRoot", false ) ) // false = optional
	{
		return false; // GetString will have emitted error
	}
	if ( !baseDir.IsEmpty() )
	{
		NodeGraph::CleanPath( baseDir, baseDirectory );
	}
	else
	{
		baseDirectory.Clear();
	}

	return true;
}

// GetExtraOutputPaths
//------------------------------------------------------------------------------
void FunctionObjectList::GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath ) const
{
	// split to individual tokens
	Array< AString > tokens;
	args.Tokenize( tokens );

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it != end; ++it )
	{
		if ( it->BeginsWithI( "/Fd" ) )
		{
			GetExtraOutputPath( it, end, "/Fd", pdbPath );
			continue;
		}

		if ( it->BeginsWithI( "/Fa" ) )
		{
			GetExtraOutputPath( it, end, "/Fa", asmPath );
			continue;
		}
	}
}

// GetExtraOutputPath
//------------------------------------------------------------------------------
void FunctionObjectList::GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path ) const
{
	const char * bodyStart = it->Get() + strlen( option );
	const char * bodyEnd = it->GetEnd();

	// if token is exactly matched then value is next token
	if ( bodyStart == bodyEnd )
	{
		++it;
		// handle missing next value
		if ( it == end )
		{
			return; // we just pretend it doesn't exist and let the compiler complain
		}

		bodyStart = it->Get();
		bodyEnd = it->GetEnd();
	}

	// Strip quotes
	Args::StripQuotes( bodyStart, bodyEnd, path );

	// If it's not already a path (i.e. includes filename.ext) then
	// truncate to just the path
	const char * lastSlash = path.FindLast( '\\' );
	lastSlash = lastSlash ? lastSlash : path.FindLast( '/' );
	lastSlash  = lastSlash ? lastSlash : path.Get(); // no slash, means it's just a filename
	if ( lastSlash != ( path.GetEnd() - 1 ) )
	{
		path.SetLength( uint32_t(lastSlash - path.Get()) );
	}
}

//------------------------------------------------------------------------------
