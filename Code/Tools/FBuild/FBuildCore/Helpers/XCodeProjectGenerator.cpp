// XCodeProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "XCodeProjectGenerator.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::XCodeProjectGenerator() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::~XCodeProjectGenerator() = default;

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
    WritePBXSourcesBuildPhase();
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

void GetGUID_PBXNativeTarget( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x33333333;
    const uint32_t part2 = 0x00000000;
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

void GetGUID_PBXSourcesBuildPhase( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x77777777;
    const uint32_t part2 = 0x77777777;
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
    const uint32_t numFiles = (uint32_t)m_Files.GetSize();

    // Files (PBXBuildFile)
    Write( "\n" );
    Write( "/* Begin PBXBuildFile section */\n" );
    for ( uint32_t fileIndex=0; fileIndex<numFiles; ++fileIndex )
    {
        const File & file = m_Files[ fileIndex ];

        // Get just the filename part from the full path
        const char * shortName = file.m_Name.FindLast( NATIVE_SLASH );
        shortName = shortName ? ( shortName + 1 ) : file.m_Name.Get();

        const char * shortFolderName = "TODO"; // TODO:

        Write( "\t\t1111111100000000%08X /* %s in %s */ = {isa = PBXBuildFile; fileRef = 1111111111111111%08X /* %s */; };\n",
                    fileIndex, shortName, shortFolderName, fileIndex, shortName );		
    }
    Write( "/* End PBXBuildFile section */\n" );        

    // Files (PBXFileReference)
    Write( "\n" );
    Write( "/* Begin PBXFileReference section */\n" );
    for ( uint32_t fileIndex=0; fileIndex<numFiles; ++fileIndex )
    {
        const File & file = m_Files[ fileIndex ];
        
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

        const char * folderName = nullptr; // root folder is unnamed
        if ( folder.m_Path.IsEmpty() == false )
        {
            folderName = folder.m_Path.FindLast( NATIVE_SLASH );
            folderName = folderName ? ( folderName + 1 ) : folder.m_Path.Get();
        }

        Write( "\t\t%s /* %s */ = {\n"
               "\t\t\tisa = PBXGroup;\n"
               "\t\t\tchildren = (\n",
               pbxGroupGUID.Get(), folderName ? folderName : "" );

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
        if ( folderName )
        {
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
    // PBXLegacyTarget
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

    // PBXNativeTarget
    {
        AStackString<> pbxNativeTargetGUID;
        GetGUID_PBXNativeTarget( 0, pbxNativeTargetGUID );

        AStackString<> xConfigurationListGUID;
        GetGUID_XConfigurationList( 2, xConfigurationListGUID ); 

        AStackString<> buildPhaseGuid;
        GetGUID_PBXSourcesBuildPhase( 0, buildPhaseGuid );

        Write( "\n" );
        Write( "/* Begin PBXNativeTarget section */\n" );
        Write( "\t\t%s /* %s-doc */ = {\n", pbxNativeTargetGUID.Get(), m_ProjectName.Get() );
        Write( "\t\t\tisa = PBXNativeTarget;\n" );
        Write( "\t\t\tbuildConfigurationList = %s /* Build configuration list for PBXNativeTarget \"%s-doc\" */;\n", xConfigurationListGUID.Get(), m_ProjectName.Get() );
        Write( "\t\t\tbuildPhases = (\n" );
        Write( "\t\t\t\t%s /* Sources */,\n", buildPhaseGuid.Get() );
        Write( "\t\t\t);\n" );
        Write( "\t\t\tbuildRules = (\n" );
        Write( "\t\t\t);\n" );
        Write( "\t\t\tdependencies = (\n" );
        Write( "\t\t\t);\n" );
        Write( "\t\t\tname = \"%s-doc\";\n", m_ProjectName.Get() );
        Write( "\t\t\tproductName = \"%s-doc\";\n", m_ProjectName.Get() );
        Write( "\t\t\tproductType = \"com.apple.product-type.tool\";\n" );        
        Write( "\t\t};\n" );
        Write( "/* End PBXNativeTarget section */\n" );
    }
}

// WriteGeneralSettings
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteGeneralSettings()
{
    AStackString<> pbxLegacyTargetGUID;
    GetGUID_PBXLegacyTarget( 0, pbxLegacyTargetGUID );

    AStackString<> pbxNativeTargetGUID;
    GetGUID_PBXNativeTarget( 0, pbxNativeTargetGUID );

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
    Write( "\t\t\t\t\t%s = {\n", pbxNativeTargetGUID.Get() );
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
    Write( "\t\t\t\t%s /* %s-doc */,\n", pbxNativeTargetGUID.Get(), m_ProjectName.Get() );
    Write( "\t\t\t);\n" );
    Write( "\t\t};\n" );
    Write( "/* End PBXProject section */\n" );
}

// WritePBXSourcesBuildPhase
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WritePBXSourcesBuildPhase()
{
    AStackString<> buildPhaseGuid;
    GetGUID_PBXSourcesBuildPhase( 0, buildPhaseGuid );

    Write( "\n" );
    Write( "/* Begin PBXSourcesBuildPhase section */\n" );

    Write( "\t\t%s /* Sources */ = {\n", buildPhaseGuid.Get());
    Write( "\t\t\tisa = PBXSourcesBuildPhase;\n"
           /*"\t\t\tbuildActionMask = 2147483647;\n"*/ ); // TODO: What is this for?

    Write( "\t\t\tfiles = (\n" );
    for ( uint32_t fileIndex=0; fileIndex<m_Files.GetSize(); ++fileIndex )
    {
        const File & file = m_Files[ fileIndex ];

        // Get just the filename part from the full path
        const char * shortName = file.m_Name.FindLast( NATIVE_SLASH );
        shortName = shortName ? ( shortName + 1 ) : file.m_Name.Get();

        const char * shortFolderName = "TODO"; // TODO:

        Write( "\t\t\t\t1111111100000000%08x /* %s in %s */,\n",
                    fileIndex, shortName, shortFolderName );        
    }
    Write( "\t\t\t);\n" );

    Write( "\t\t\trunOnlyForDeploymentPostprocessing = 0;\n" );

    Write( "\t\t};\n" );
    Write( "/* End PBXSourcesBuildPhase section */\n" );
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

        const AString& target = config.m_TargetNode ? config.m_TargetNode->GetName() : AString::GetEmpty();
        Write( "\t\t%s /* %s */ = {\n"
                "\t\t\tisa = XCBuildConfiguration;\n"
                "\t\t\tbuildSettings = {\n"
                "\t\t\t\tDEBUG_WORKING_DIR = \"%s\";\n"
                "\t\t\t\tFASTBUILD_TARGET = \"%s\";\n"
                "\t\t\t};\n"
                "\t\t\tname = %s;\n"
                "\t\t};\n",
                xcBuildConfigurationGUID.Get(), config.m_Name.Get(), debugWorkingDir, target.Get(), config.m_Name.Get() );
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

    configId = 200;
    for ( const auto & config : m_Configs )
    {
        AStackString<> xcBuildConfigurationGUID;
        GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
        ++configId;

        Write( "\t\t%s /* %s */ = {\n"
               "\t\t\tisa = XCBuildConfiguration;\n"
               "\t\t\tbuildSettings = {\n",
               xcBuildConfigurationGUID.Get(), config.m_Name.Get() );

        // TODO:B Can this (and other warning settings) be derived from the compiler options automatically?
        Write( "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";\n" );

        // Find target from which to extract Intellisense options
        const ObjectListNode * oln = ProjectGeneratorBase::FindTargetForIntellisenseInfo( config.m_TargetNode );
        if ( oln )
        {
            // Defines
            {
                Array< AString > defines;
                ProjectGeneratorBase::ExtractIntellisenseOptions( oln->GetCompilerOptions(), "/D", "-D", defines, true );
                AStackString<> definesStr;
                ProjectGeneratorBase::ConcatIntellisenseOptions( defines, definesStr, "\t\t\t\t\t\"", "\",\n" );
                Write( "\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (\n" );
                Write( "%s", definesStr.Get() );
                //Write( "\t\t\t\t\t\"MONOLITHIC_BUILD=1\",\n" ); // TODO:C Is this needed?
                Write( "\t\t\t\t);\n" );
            }

            // System Include Paths
            // TODO:B Do we need to differentiate include path types? 
            {
                //Write( "\t\t\t\tHEADER_SEARCH_PATHS = (\n" );
                //Write( "%s", includePathsStr.Get() );
                //Write( "\t\t\t\t);\n" );
            }

            // User Include Paths
            {
                Array< AString > includePaths;
                ProjectGeneratorBase::ExtractIntellisenseOptions( oln->GetCompilerOptions(), "/I", "-I", includePaths, true );
                for ( AString & include : includePaths )
                {
                    AStackString<> fullIncludePath;
                    NodeGraph::CleanPath( include, fullIncludePath ); // Expand to full path - TODO:C would be better to be project relative
                    include = fullIncludePath;
                }
                AStackString<> includePathsStr;
                ProjectGeneratorBase::ConcatIntellisenseOptions( includePaths, includePathsStr, "\t\t\t\t\t\"", "\",\n" );
                Write( "\t\t\t\tUSER_HEADER_SEARCH_PATHS = (\n" );
                Write( "%s", includePathsStr.Get() );
                Write( "\t\t\t\t);\n" );
            }
        }

        Write( "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";\n"
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
    const char * sections[3] = { "PBXProject", "PBXLegacyTarget", "PBXNativeTarget" };
    const char * ext[3] = { "", "", "-doc" };
    const uint32_t configStartIds[3] = { 0, 100, 200 };

    Write( "\n" );
    Write( "/* Begin XCConfigurationList section */\n" );
    for ( uint32_t i=0; i<3; ++i )
    {
        AStackString<> xConfigurationListGUID;
        GetGUID_XConfigurationList( i, xConfigurationListGUID );

        Write( "\t\t%s /* Build configuration list for %s \"%s%s\" */ = {\n", xConfigurationListGUID.Get(), sections[ i ], m_ProjectName.Get(), ext[ i ] );
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
        if ( i != 2 )
        {
            Write( "\t\t\tdefaultConfigurationName = %s;\n", m_Configs[ 0 ].m_Name.Get() ); // Use first config as default
        }
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
