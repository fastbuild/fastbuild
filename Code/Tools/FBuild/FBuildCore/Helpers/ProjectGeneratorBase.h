// ProjectGeneratorBase
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------

// ProjectGeneratorBase
//------------------------------------------------------------------------------
class ProjectGeneratorBase
{
public:
	ProjectGeneratorBase();
	~ProjectGeneratorBase();
	
	inline void SetBasePaths( const Array< AString > & paths ) { m_BasePaths = paths; }
	void AddFile( const AString & fileName );
	
	void AddConfig( const AString & name, const AString & target );

	static bool	WriteIfDifferent( const char * generatorId, const AString & content, const AString & fileName );

	static void GetDefaultAllowedFileExtensions( Array< AString > & extensions );
	static void FixupAllowedFileExtensions( Array< AString > & extensions );
protected:
	// Helper to format some text
	void Write( const char * fmtString, ... );
	
	// Internal helpers
	void 		GetProjectRelativePath( const AString & fileName, AString & shortFileName ) const;
	uint32_t 	GetFolderIndexFor( const AString & path );

	struct Folder
	{
		AString 			m_Path;		// Project Base Path(s) relative
		Array< uint32_t >	m_Files;	// Indices into m_Files
		Array< uint32_t >	m_Folders;	// Indices into m_Folders
	};
	struct File
	{
		AString 	m_Name;			// Project Base Path(s) relative
		AString		m_FullPath;		// Full path
		uint32_t	m_FolderIndex;	// Index into m_Folders
	};
	struct Config
	{
		AString		m_Name;			// Config name (e.g. Debug, Release etc.)
		AString		m_Target;		// Target to pass on cmdline to FASTBuild
	};

	// Input Data
	Array< AString >	m_BasePaths;
	Array< Folder >		m_Folders;
	Array< File >		m_Files;
	Array< Config >		m_Configs;

	// working buffer
	AString m_Tmp;
};

//------------------------------------------------------------------------------
