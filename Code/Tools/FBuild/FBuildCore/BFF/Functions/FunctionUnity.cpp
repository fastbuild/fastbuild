// FunctionUnity
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionUnity.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

// Core
#include "Core/Strings/AStackString.h"
#include "Core/FileIO/PathUtils.h"

// UnityNode
//------------------------------------------------------------------------------
class UnityNode;

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionUnity::FunctionUnity()
: Function( "Unity" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::NeedsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::Commit( const BFFIterator & funcStartIter ) const
{
	Array< AString > paths;
	Array< AString > pathsToExclude;
	Array< AString > excludePatterns;
	const BFFVariable * inputPatternV;
	bool inputPathRecurse;
	const BFFVariable * outputPathV;
	const BFFVariable * outputPatternV;
	bool isolateWritableFiles;
	Array< AString > files;
	int numFiles;
	int maxIsolatedFiles;
	if ( !GetStrings( funcStartIter, pathsToExclude, ".UnityInputExcludePath", false ) ||
		 !GetStrings( funcStartIter, excludePatterns, ".UnityInputExcludePattern", false ) ||
		 !GetString( funcStartIter, inputPatternV,	".UnityInputPattern" ) ||
		 !GetBool( funcStartIter, inputPathRecurse,".UnityInputPathRecurse", true ) ||
		 !GetStrings( funcStartIter, files, ".UnityInputFiles", false ) ||
		 !GetString( funcStartIter, outputPathV,	".UnityOutputPath", true ) ||
		 !GetString( funcStartIter, outputPatternV,	".UnityOutputPattern" ) ||
		 !GetStrings( funcStartIter, paths, ".UnityInputPath", false ) ||
		 !GetBool( funcStartIter, isolateWritableFiles,".UnityInputIsolateWritableFiles", false ) ||
		 !GetInt( funcStartIter, numFiles, ".UnityNumFiles", 1, false, 1, 999 ) ||
		 !GetInt( funcStartIter, maxIsolatedFiles, ".UnityInputIsolateWritableFilesLimit", 0, false, 0, 999 ) )
	{
		return false;
	}

	// clean exclude paths
	CleanFolderPaths( paths );
	CleanFolderPaths( pathsToExclude );	// exclude paths
	CleanFilePaths( files );			// explicit file list

	// clean exclude patterns (just slashes)
	for ( Array< AString >::Iter it = excludePatterns.Begin(); it != excludePatterns.End(); ++it )
	{
		it->Replace( OTHER_SLASH, NATIVE_SLASH );
	}

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	const AString & inputPattern = inputPatternV ? inputPatternV->GetString() : (const AString &)AStackString<>( "*.cpp" );
	const AString & outputPattern = outputPatternV ? outputPatternV->GetString() : (const AString &)AStackString<>( "Unity*.cpp" );

	Dependencies dirNodes( paths.GetSize() );
	if ( !GetDirectoryListNodeList( funcStartIter, paths, pathsToExclude, inputPathRecurse, inputPattern, "UnityInputPath", dirNodes ) )
	{
		return false; // GetDirectoryListNodeList will have emitted an error
	}

	// uses precompiled header?
	AStackString< 512 > precompiledHeader;
	const BFFVariable * pchV = nullptr;
	if ( !GetString( funcStartIter, pchV, ".UnityPCH" ) )
	{
		return false;
	}
	if ( pchV )
	{
		// Note: leave this as a relative path
		precompiledHeader = pchV->GetString();
        PathUtils::FixupFilePath( precompiledHeader );
	}

	// determine files to exclude
	Array< AString > filesToExclude( 0, true );
	if ( !GetStrings( funcStartIter, filesToExclude, ".UnityInputExcludedFiles", false ) ) // not required
	{
		return false; // GetStrings will have emitted an error
	}
	// cleanup slashes (keep path relative)
	AString * const end = filesToExclude.End();
	for ( AString * it = filesToExclude.Begin(); it != end; ++it )
	{
		AString & file = *it;

		// normalize slashes
		PathUtils::FixupFilePath( file );

		// ensure file starts with slash, so we only match full paths
		if ( PathUtils::IsFullPath( file ) == false )
		{
			AStackString<> tmp;
			tmp += NATIVE_SLASH;
			tmp += file;
			file = tmp;
		}
	}

	// automatically exclude the associated CPP file for a PCH (if there is one)
	if ( precompiledHeader.EndsWithI( ".h" ) )
	{
		AStackString<> pchCPP( precompiledHeader.Get(), 
							   precompiledHeader.Get() + precompiledHeader.GetLength() - 2 );
		pchCPP += ".cpp";
		filesToExclude.Append( pchCPP );
	}

	// parsing logic should guarantee we have a string for our name
	ASSERT( m_AliasForFunction.IsEmpty() == false );

	AStackString< 512 > fullOutputPath;
	NodeGraph::CleanPath( outputPathV->GetString(), fullOutputPath );
	PathUtils::EnsureTrailingSlash( fullOutputPath );

	// Check for existing node
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	UnityNode * un = ng.CreateUnityNode( m_AliasForFunction, // name
										 dirNodes,
										 files,
										 fullOutputPath,
										 outputPattern,
										 numFiles,
										 precompiledHeader,
										 pathsToExclude, // TODO:B Remove this (handled by DirectoryListNode now)
										 filesToExclude,
										 isolateWritableFiles,
										 maxIsolatedFiles,
										 excludePatterns );
	ASSERT( un ); (void)un;

	return true;
}

//------------------------------------------------------------------------------
