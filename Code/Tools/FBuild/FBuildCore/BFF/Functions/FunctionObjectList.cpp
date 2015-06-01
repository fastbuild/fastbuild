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
/*virtual*/ bool FunctionObjectList::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	const BFFVariable * compiler;
	const BFFVariable * compilerOptions;
	AStackString<> compilerOptionsDeoptimized;
	AStackString<> compilerOutputPath;
	const BFFVariable * compilerOutputExtension;
	if ( !GetString( funcStartIter, compiler, ".Compiler", true ) ||
		 !GetString( funcStartIter, compilerOptions, ".CompilerOptions", true ) ||
		 !GetString( funcStartIter, compilerOptionsDeoptimized, ".CompilerOptionsDeoptimized", false ) ||
		 !GetString( funcStartIter, compilerOutputPath, ".CompilerOutputPath", true ) ||
		 !GetString( funcStartIter, compilerOutputExtension, ".CompilerOutputExtension", false ) )
	{
		return false;
	}

    PathUtils::FixupFolderPath( compilerOutputPath );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// find or create the compiler node
	CompilerNode * compilerNode = nullptr;
	if ( !FunctionObjectList::GetCompilerNode( funcStartIter, compiler->GetString(), compilerNode ) )
	{
		return false; // GetCompilerNode will have emitted error
	}

	// Sanity check compile flags
	uint32_t objFlags = ObjectNode::DetermineFlags( compilerNode, compilerOptions->GetString() );
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
            if ( args.Find( "/c" ) == nullptr )
            {
		        Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".CompilerOptions", "/c" );
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

	// Compiler Force Using
	Dependencies compilerForceUsing;
	if ( !GetNodeList( funcStartIter, ".CompilerForceUsing", compilerForceUsing, false ) )
	{
		return false; // GetNodeList will have emitted an error
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
        if ( !FunctionObjectList::GetCompilerNode( funcStartIter, preprocessor->GetString(), preprocessorNode ) )
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
	if ( !GetNodeList( funcStartIter, ".PreBuildDependencies", preBuildDependencies, false ) )
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

	// Precompiled Header support
	ObjectNode * precompiledHeaderNode = nullptr;
	if ( !GetPrecompiledHeaderNode( funcStartIter, compilerNode, objFlags, compilerOptions, compilerForceUsing, precompiledHeaderNode, deoptimizeWritableFiles, deoptimizeWritableFilesWithToken ) )
	{
		return false; // GetPrecompiledHeaderNode will have emitted error
	}

	Dependencies staticDeps( 32, true );
	if ( !GetInputs( funcStartIter, staticDeps ) )
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
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	// Create library node which depends on the single file or list
	ObjectListNode * o = ng.CreateObjectListNode( m_AliasForFunction,
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
                                                  preprocessorNode,
                                                  preprocessorOptions ? preprocessorOptions->GetString() : AString::GetEmpty() );
	if ( compilerOutputExtension )
	{
		o->m_ObjExtensionOverride = compilerOutputExtension->GetString();
	}

	return true;
}

// GetCompilerNode
//------------------------------------------------------------------------------
bool FunctionObjectList::GetCompilerNode( const BFFIterator & iter, const AString & compiler, CompilerNode * & compilerNode ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * cn = ng.FindNode( compiler );
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
#ifdef USE_NODE_REFLECTION
		compilerNode = ng.CreateCompilerNode( compiler );
        VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "AllowDistribution", false ) );
#else
		const bool allowDistribution = false;
		compilerNode = ng.CreateCompilerNode( compiler, Dependencies( 0, false ), allowDistribution );
#endif
	}

	return true;
}

// GetPrecompiledHeaderNode
//------------------------------------------------------------------------------
bool FunctionObjectList::GetPrecompiledHeaderNode( const BFFIterator & iter,
												   CompilerNode * compilerNode,
												   uint32_t objFlags,
												   const BFFVariable * compilerOptions,
												   const Dependencies & compilerForceUsing,
												   ObjectNode * & precompiledHeaderNode,
												   bool deoptimizeWritableFiles,
												   bool deoptimizeWritableFilesWithToken ) const
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

	if ( pchInputFile ) 
	{
		if ( !pchOutputFile || !pchOptions )
		{
			Error::Error_1300_MissingPCHArgs( iter, this );
			return false;
		}

		AStackString<> pchOptionsDeoptimized;
		if ( !GetString( iter, pchOptionsDeoptimized, ".PCHOptionsDeoptimized", ( deoptimizeWritableFiles || deoptimizeWritableFilesWithToken ) ) )
		{
			return false;
		}

		NodeGraph & ng = FBuild::Get().GetDependencyGraph();
		Node * pchInputNode = ng.FindNode( pchInputFile->GetString() );
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
			pchInputNode = ng.CreateFileNode( pchInputFile->GetString() );
		}

		if ( ng.FindNode( pchOutputFile->GetString() ) )
		{
			Error::Error_1301_AlreadyDefinedPCH( iter, this, pchOutputFile->GetString().Get() );
			return false;
		}

		uint32_t pchFlags = ObjectNode::DetermineFlags( compilerNode, pchOptions->GetString() );
		if ( pchFlags & ObjectNode::FLAG_MSVC )
		{
			// sanity check arguments

			// PCH must have "Create PCH" (e.g. /Yc"PrecompiledHeader.h")
			if ( !( pchFlags & ObjectNode::FLAG_CREATING_PCH ) )
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
			if ( pchOptions->GetString().Find( "/Fo" ) == nullptr )
			{
				Error::Error_1302_MissingPCHCompilerOption( iter, this, "/Fo", "PCHOptions" );
				return false;
			}

			// Object using the PCH must have "Use PCH" option (e.g. /Yu"PrecompiledHeader.h")
			if ( !( objFlags & ObjectNode::FLAG_USING_PCH ) )
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

		// TODO:B Check PCHOptionsDeoptimized

		precompiledHeaderNode = ng.CreateObjectNode( pchOutputFile->GetString(),
													 pchInputNode,
													 compilerNode,
													 pchOptions->GetString(),
													 pchOptionsDeoptimized,
													 nullptr,
													 pchFlags,
													 compilerForceUsing,
													 deoptimizeWritableFiles,
													 deoptimizeWritableFilesWithToken,
                                                     nullptr, AString::GetEmpty(), 0 ); // preprocessor args not supported
	}

	return true;
}

// GetInputs
//------------------------------------------------------------------------------
bool FunctionObjectList::GetInputs( const BFFIterator & iter, Dependencies & inputs ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// do we want to build files via a unity blob?
	const BFFVariable * inputUnity = nullptr;
	if ( !GetString( iter, inputUnity, ".CompilerInputUnity" ) )
	{
		return false;
	}
	if ( inputUnity )
	{
		Node * n = ng.FindNode( inputUnity->GetString() );
		if ( n == nullptr )
		{
			Error::Error_1104_TargetNotDefined( iter, this, "CompilerInputUnity", inputUnity->GetString() );
			return false;
		}
		if ( n->GetType() != Node::UNITY_NODE )
		{
			Error::Error_1102_UnexpectedType( iter, this, "CompilerInputUnity", inputUnity->GetString(), n->GetType(), Node::UNITY_NODE );
			return false;
		}
		inputs.Append( Dependency( n ) );
	}

	// do we want to build a files in a directory?
	const BFFVariable * inputPath = BFFStackFrame::GetVar( ".CompilerInputPath" );
	if ( inputPath )
	{
		// get the optional pattern and recurse options related to InputPath
		const BFFVariable * patternVar = nullptr;
		if ( !GetString( iter, patternVar, ".CompilerInputPattern", false ) )
		{
			return false; // GetString will have emitted an error
		}
		AStackString<> defaultWildCard( "*.cpp" );
		const AString & pattern = patternVar ? patternVar->GetString() : defaultWildCard;

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
		if ( !GetDirectoryListNodeList( iter, inputPaths, excludePaths, filesToExclude, recurse, pattern, "CompilerInputPath", dirNodes ) )
		{
			return false; // GetDirectoryListNodeList will have emitted an error
		}
		inputs.Append( dirNodes );
	}

	// do we want to build a specific list of files?
	if ( !GetNodeList( iter, ".CompilerInputFiles", inputs, false ) )
	{
		// helper will emit error
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
