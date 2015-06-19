// FunctionLibrary
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionLibrary.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionLibrary::FunctionLibrary()
: FunctionObjectList()
{
	m_Name = "Library";
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::NeedsHeader() const
{
	return false;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	const BFFVariable * outputLib;
	const BFFVariable * compiler;
	const BFFVariable * compilerOptions;
	AStackString<> compilerOptionsDeoptimized;
	AStackString<> compilerOutputPath;
    AStackString<> compilerOutputPrefix;
	const BFFVariable * compilerOutputExtension;
	const BFFVariable * librarian;
	const BFFVariable * librarianOptions;
	if ( !GetString( funcStartIter, outputLib, ".LibrarianOutput", true ) ||
		 !GetString( funcStartIter, compiler, ".Compiler", true ) ||
		 !GetString( funcStartIter, compilerOptions, ".CompilerOptions", true ) ||
		 !GetString( funcStartIter, compilerOptionsDeoptimized, ".CompilerOptionsDeoptimized", false ) ||
		 !GetString( funcStartIter, compilerOutputPath, ".CompilerOutputPath", true ) ||
		 !GetString( funcStartIter, compilerOutputPrefix, ".CompilerOutputPrefix", false ) ||
		 !GetString( funcStartIter, compilerOutputExtension, ".CompilerOutputExtension", false ) ||
		 !GetString( funcStartIter, librarian, ".Librarian", true ) ||
		 !GetString( funcStartIter, librarianOptions, ".LibrarianOptions", true ) )
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

	// Check input/output for Librarian
	{
		const AString & args = librarianOptions->GetString();
		bool hasInputToken = ( args.Find( "%1" ) || args.Find( "\"%1\"" ) );
		if ( hasInputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".LibrarianOptions", "%1" );
			return false;
		}
		bool hasOutputToken = ( args.Find( "%2" ) || args.Find( "\"%2\"" ) );
		if ( hasOutputToken == false )
		{
			Error::Error_1106_MissingRequiredToken( funcStartIter, this, ".LibrarianOptions", "%2" );
			return false;
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

	// are the additional inputs to merge into the libaray?
	Dependencies additionalInputs;
	if ( !GetNodeList( funcStartIter, ".LibrarianAdditionalInputs", additionalInputs, false ) )
	{
		return false;// helper will emit error
	}

	if ( staticDeps.IsEmpty() && additionalInputs.IsEmpty() )
	{
		Error::Error_1006_NothingToBuild( funcStartIter, this );
		return false;
	}

	uint32_t flags = LibraryNode::DetermineFlags( librarian->GetString() );

	// Create library node which depends on the single file or list
	if ( ng.FindNode( outputLib->GetString() ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, outputLib->GetString() );
		return false;
	}
	LibraryNode * libNode = ng.CreateLibraryNode( outputLib->GetString(),
						  staticDeps,
						  compilerNode,
						  compilerOptions->GetString(),
						  compilerOptionsDeoptimized,
						  compilerOutputPath,
						  librarian->GetString(),
						  librarianOptions->GetString(),
						  flags,
						  precompiledHeaderNode,
						  compilerForceUsing,
						  preBuildDependencies,
						  additionalInputs,
						  deoptimizeWritableFiles,
						  deoptimizeWritableFilesWithToken,
                          preprocessorNode,
                          preprocessorOptions ? preprocessorOptions->GetString() : AString::GetEmpty() );
	if ( compilerOutputExtension )
	{
		libNode->m_ObjExtensionOverride = compilerOutputExtension->GetString();
	}
    libNode->m_CompilerOutputPrefix = compilerOutputPrefix;

	return ProcessAlias( funcStartIter, libNode );
}

//------------------------------------------------------------------------------
