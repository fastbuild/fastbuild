// XCodeProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "XCodeProjectGenerator.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::XCodeProjectGenerator()
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::~XCodeProjectGenerator()
{
}

// Generate
//------------------------------------------------------------------------------
const AString & XCodeProjectGenerator::Generate()
{
	// preallocate to avoid re-allocations
	m_Tmp.SetReserved( MEGABYTE );
	m_Tmp.SetLength( 0 );

	WriteHeader();
	WriteFiles();
	WriteFolders();
	WriteBuildCommand();
	WriteGeneralSettings();
	WriteBuildConfiguration();
	WriteConfigurationList();
	WriteFooter();
	
	return m_Tmp;
}

void GetGUID_PBXGroup( const uint32_t index, AString & outGUID )
{
	const uint32_t part1 = 0x22222222;
	const uint32_t part2 = 0x22222222;
	const uint32_t part3 = index;
	outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void GetGUID_PBXLegacyTarget( const uint32_t index, AString & outGUID )
{
	const uint32_t part1 = 0x33333333;
	const uint32_t part2 = 0x33333333;
	const uint32_t part3 = index;
	outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void GetGUID_PBXProject( const uint32_t index, AString & outGUID )
{
	const uint32_t part1 = 0x44444444;
	const uint32_t part2 = 0x44444444;
	const uint32_t part3 = index;
	outGUID.Format( "%08X%08X%08X", part1, part2, part3 );	
}

void GetGUID_XConfigurationList( const uint32_t index, AString & outGUID )
{
	const uint32_t part1 = 0x55555555;
	const uint32_t part2 = 0x55555555;
	const uint32_t part3 = index;
	outGUID.Format( "%08X%08X%08X", part1, part2, part3 );	
}

void GetGUID_XCBuildConfiguration( const uint32_t index, AString & outGUID )
{
	const uint32_t part1 = 0x66666666;
	const uint32_t part2 = 0x66666666;
	const uint32_t part3 = index;
	outGUID.Format( "%08X%08X%08X", part1, part2, part3 );	
}

// WriteHeader
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteHeader()
{
	// Project Header
	Write( "// !$*UTF8*$!\n"
		   "{\n"
		   "\tarchiveVersion = 1;\n"
		   "\tclasses = {\n"
		   "\t};\n"
		   "\tobjectVersion = 46;\n"
		   "\tobjects = {\n" );
}

// WriteFiles
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteFiles()
{
	// Files
	Write( "\n" );
	Write( "/* Begin PBXFileReference section */\n" );
	uint32_t fileIndex = 0;
   	for ( const File & file : m_Files )
   	{
		// Get just the filename part from the full path
		const char * shortName = file.m_Name.FindLast( NATIVE_SLASH );
		shortName = shortName ? ( shortName + 1 ) : file.m_Name.Get();
		
		// work out file type based on extension
		// TODO: What is the definitive list of these?
		const char * lastKnownFileType = "sourcecode.cpp.cpp";
		const char * fileEncoding = " fileEncoding = 4;";
		if ( file.m_Name.EndsWithI( ".h" ) )
		{
			lastKnownFileType = "sourcecode.c.h";
		}
		else if ( file.m_Name.EndsWithI( ".xcodeproj" ))
		{
			lastKnownFileType = "wrapper.pb-project";			
			fileEncoding = "";
		}		
		
 		Write( "\t\t1111111111111111%08X /* %s */ = {isa = PBXFileReference;%s lastKnownFileType = %s; name = %s; path = %s; sourceTree = \"<group>\"; };\n",
			 		fileIndex, shortName, fileEncoding, lastKnownFileType, shortName, file.m_FullPath.Get() );
		++fileIndex;
	}
	
	Write( "/* End PBXFileReference section */\n" );
}

// WriteFolders
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteFolders()
{
	// TODO:B Sort folders alphabetically
	
	// Folders
	Write( "\n" );
	Write( "/* Begin PBXGroup section */\n" );
	uint32_t folderIndex = 0;
	for ( const Folder & folder : m_Folders )
	{
		AStackString<> pbxGroupGUID; 
		GetGUID_PBXGroup( folderIndex, pbxGroupGUID );
		++folderIndex;
		
		Write( "\t\t%s = {\n"
			   "\t\t\tisa = PBXGroup;\n"
			   "\t\t\tchildren = (\n",
			   pbxGroupGUID.Get() );
			   
		// Child Files
		for ( const uint32_t fileIndex : folder.m_Files )
		{
			const File & file = m_Files[ fileIndex ];
			const char * shortName = file.m_Name.FindLast( NATIVE_SLASH );
			shortName = shortName ? ( shortName + 1 ) : file.m_Name.Get();
			Write( "\t\t\t\t1111111111111111%08X /* %s */,\n",
				fileIndex, shortName );
		}
		
		// Child Folders
		for ( const uint32_t childFolderIndex : folder.m_Folders )
		{
			const Folder & childFolder = m_Folders[ childFolderIndex ];
			const char * shortName = childFolder.m_Path.FindLast( NATIVE_SLASH );
			shortName = shortName ? ( shortName + 1 ) : childFolder.m_Path.Get();
			AStackString<> pbxGroupGUIDChild; 
			GetGUID_PBXGroup( childFolderIndex, pbxGroupGUIDChild );
			Write( "\t\t\t\t%s /* %s */,\n", pbxGroupGUIDChild.Get(), shortName );
		}
				
		Write( "\t\t\t);\n" );
	
		// Write FolderName if not the root folder
		if ( folder.m_Path.IsEmpty() == false )
		{
			const char * folderName = folder.m_Path.FindLast( NATIVE_SLASH );
			folderName = folderName ? ( folderName + 1 ) : folder.m_Path.Get();
			Write( "\t\t\tname = %s;\n", folderName );
		}
		
		Write( "\t\t\tsourceTree = \"<group>\";\n"
		 	   "\t\t};\n" );
	}
	Write( "/* End PBXGroup section */\n" );
}

// WriteBuildCommand
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteBuildCommand()
{
	AStackString<> pbxLegacyTargetGUID;	
	GetGUID_PBXLegacyTarget( 0, pbxLegacyTargetGUID );
	
	AStackString<> xConfigurationListGUID;
	GetGUID_XConfigurationList( 1, xConfigurationListGUID );	
	
	Write( "\n" );
	Write( "/* Begin PBXLegacyTarget section */\n" );
	Write( "\t\t%s /* %s */ = {\n", pbxLegacyTargetGUID.Get(), m_ProjectName.Get() );
	Write( "\t\t\tisa = PBXLegacyTarget;\n" );
	Write( "\t\t\tbuildArgumentsString = \"%s\";\n", m_XCodeBuildToolArgs.Get() );
	Write( "\t\t\tbuildConfigurationList = %s /* Build configuration list for PBXLegacyTarget \"%s\" */;\n", xConfigurationListGUID.Get(), m_ProjectName.Get() );
	Write( "\t\t\tbuildPhases = (\n" );
	Write( "\t\t\t);\n" );
	Write( "\t\t\tbuildToolPath = \"%s\";\n", m_XCodeBuildToolPath.Get() );
	Write( "\t\t\tbuildWorkingDirectory = \"%s\";\n", m_XCodeBuildWorkingDir.Get() );
	Write( "\t\t\tdependencies = (\n" );
	Write( "\t\t\t);\n" );
	Write( "\t\t\tname = %s;\n", m_ProjectName.Get() );
	Write( "\t\t\tpassBuildSettingsInEnvironment = 1;\n" );
	Write( "\t\t\tproductName = %s;\n", m_ProjectName.Get() );
	Write( "\t\t};\n" );
	Write( "/* End PBXLegacyTarget section */\n" );
}

// WriteGeneralSettings
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteGeneralSettings()
{	
	AStackString<> pbxLegacyTargetGUID;	
	GetGUID_PBXLegacyTarget( 0, pbxLegacyTargetGUID );	
	
	AStackString<> pbxProjectGUID;	
	GetGUID_PBXProject( 0, pbxProjectGUID );
	
	AStackString<> xConfigurationListGUID;
	GetGUID_XConfigurationList( 0, xConfigurationListGUID );	

	AStackString<> pbxGroupGUID; 
	GetGUID_PBXGroup( 0, pbxGroupGUID );
	
	Write( "\n" );
	Write( "/* Begin PBXProject section */\n" );
	Write( "\t\t%s /* Project object */ = {\n", pbxProjectGUID.Get() );
	Write( "\t\t\tisa = PBXProject;\n" );
	Write( "\t\t\tattributes = {\n" );
	Write( "\t\t\t\tLastUpgradeCheck = 0630;\n" );
	Write( "\t\t\t\tORGANIZATIONNAME = \"%s\";\n", m_XCodeOrganizationName.Get() );
	Write( "\t\t\t\tTargetAttributes = {\n" );
	Write( "\t\t\t\t\t%s = {\n", pbxLegacyTargetGUID.Get() );
	Write( "\t\t\t\t\t\tCreatedOnToolsVersion = 6.3.2;\n" );
	Write( "\t\t\t\t\t};\n" );
	Write( "\t\t\t\t};\n" );
	Write( "\t\t\t};\n" );
	Write( "\t\t\tbuildConfigurationList = %s /* Build configuration list for PBXProject \"%s\" */;\n", xConfigurationListGUID.Get(), m_ProjectName.Get() );
	Write( "\t\t\tcompatibilityVersion = \"Xcode 3.2\";\n" );
	Write( "\t\t\tdevelopmentRegion = English;\n" );
	Write( "\t\t\thasScannedForEncodings = 0;\n" );
	Write( "\t\t\tknownRegions = (\n" );
	Write( "\t\t\t\ten,\n" );
	Write( "\t\t\t);\n" );
	Write( "\t\t\tmainGroup = %s;\n", pbxGroupGUID.Get() );
	Write( "\t\t\tprojectDirPath = \"\";\n" );
	Write( "\t\t\tprojectRoot = \"\";\n" );
	Write( "\t\t\ttargets = (\n" );
	Write( "\t\t\t\t%s /* %s */,\n", pbxLegacyTargetGUID.Get(), m_ProjectName.Get() );
	Write( "\t\t\t);\n" );
	Write( "\t\t};\n" );
	Write( "/* End PBXProject section */\n" );
}

// WriteBuildConfiguration
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteBuildConfiguration()
{
	const char * debugWorkingDir = "/Users/ffulin/p4/Code/"; // TODO: Expose & per-config

	Write( "\n" );
	Write( "/* Begin XCBuildConfiguration section */\n" );
	
	uint32_t configId = 0;
	for ( const auto & config : m_Configs )
	{
		AStackString<> xcBuildConfigurationGUID;
		GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
		++configId;
		
		Write( "\t\t%s /* %s */ = {\n"
				"\t\t\tisa = XCBuildConfiguration;\n"
				"\t\t\tbuildSettings = {\n"
				"\t\t\t\tDEBUG_WORKING_DIR = \"%s\";\n"
				"\t\t\t\tFASTBUILD_TARGET = \"%s\";\n"
				"\t\t\t};\n"
				"\t\t\tname = %s;\n"
				"\t\t};\n",
				xcBuildConfigurationGUID.Get(), config.m_Name.Get(), debugWorkingDir, config.m_Target.Get(), config.m_Name.Get() );
	}
	
	configId = 100;
	for ( const auto & config : m_Configs )
	{
		AStackString<> xcBuildConfigurationGUID;
		GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
		++configId;
		
		Write( "\t\t%s /* %s */ = {\n"
 			   "\t\t\tisa = XCBuildConfiguration;\n"
			   "\t\t\tbuildSettings = {\n",
			   xcBuildConfigurationGUID.Get(), config.m_Name.Get() );
		Write( "\t\t\t\tOTHER_CFLAGS = \"\";\n"
			   "\t\t\t\tOTHER_LDFLAGS = \"\";\n"
			   "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";\n"
			   "\t\t\t};\n"
			   "\t\t\tname = %s;\n"
			   "\t\t};\n",
			   config.m_Name.Get() );
	}
	
	Write( "/* End XCBuildConfiguration section */\n" );
}

// WriteConfigurationList
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteConfigurationList()
{
	const char * sections[2] = { "PBXProject", "PBXLegacyTarget" };
	const uint32_t ids[2] = { 0, 1 };
	const uint32_t configStartIds[2] = { 0, 100 };
	
	Write( "\n" );
	Write( "/* Begin XCConfigurationList section */\n" );
	for ( uint32_t i=0; i<2; ++i )
	{
		AStackString<> xConfigurationListGUID;
		GetGUID_XConfigurationList( ids[ i ], xConfigurationListGUID );
		
		Write( "\t\t%s /* Build configuration list for %s \"%s\" */ = {\n", xConfigurationListGUID.Get(), sections[ i ], m_ProjectName.Get() );
		Write( "\t\t\tisa = XCConfigurationList;\n" );
		Write( "\t\t\tbuildConfigurations = (\n" );
		uint32_t configId( configStartIds[ i ] );
		for ( const auto & config : m_Configs )
		{
			AStackString<> xcBuildConfigurationGUID;
			GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
			++configId;
			
			Write( "\t\t\t\t%s /* %s */,\n", xcBuildConfigurationGUID.Get(), config.m_Name.Get() );
		}
		Write( "\t\t\t);\n" );
		Write( "\t\t\tdefaultConfigurationIsVisible = 0;\n" );
		Write( "\t\t\tdefaultConfigurationName = %s;\n", m_Configs[ 0 ].m_Name.Get() ); // Use first config as default
		Write( "\t\t};\n" );
	}
	Write( "/* End XCConfigurationList section */\n" );
}

// WriteFooter
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteFooter()
{
	AStackString<> pbxProjectGUID;	
	GetGUID_PBXProject( 0, pbxProjectGUID );
	
	Write( "\t};\n"
		   "\trootObject = %s /* Project object */;\n"
		   "}\n",
		   pbxProjectGUID.Get() );
}

//------------------------------------------------------------------------------
