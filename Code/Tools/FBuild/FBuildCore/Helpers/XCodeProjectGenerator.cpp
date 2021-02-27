// XCodeProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "XCodeProjectGenerator.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::XCodeProjectGenerator() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectGenerator::~XCodeProjectGenerator() = default;

// GeneratePBXProj
//------------------------------------------------------------------------------
const AString & XCodeProjectGenerator::GeneratePBXProj()
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    // Sort for Scheme drop down menu and Project Navigator
    ProjectGeneratorBase::SortFilesAndFolders();

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

// GenerateUserSchemeMangementPList
//------------------------------------------------------------------------------
const AString & XCodeProjectGenerator::GenerateUserSchemeMangementPList()
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    AStackString<> pbxNativeTargetGUID;
    GetGUID_PBXNativeTarget( 0, pbxNativeTargetGUID );

    // Header
    m_Tmp = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
            "<plist version=\"1.0\">\n"
            "<dict>\n";

    m_Tmp.AppendFormat( "\t<key>SchemeUserState</key>\n"
                        "\t<dict>\n"
                        "\t\t<key>%s-doc.xcscheme_^#shared#^_</key>\n"
                        "\t\t<dict>\n"
                        "\t\t\t<key>isShown</key>\n"
                        "\t\t\t<false/>\n" // NOTE: isShown set to false
                        "\t\t</dict>\n"
                        "\t\t<key>%s.xcscheme_^#shared#^_</key>\n"
                        "\t\t<dict>\n"
                        "\t\t</dict>\n"
                        "\t</dict>\n",
                        m_ProjectName.Get(), m_ProjectName.Get() );

    m_Tmp.AppendFormat( "\t<key>SuppressBuildableAutocreation</key>\n"
                        "\t<dict>\n"
                        "\t\t<key>%s</key>\n"
                        "\t\t<dict>\n"
                        "\t\t\t<key>primary</key>\n"
                        "\t\t\t<true/>\n"
                        "\t\t</dict>\n"
                    	"\t</dict>\n",
                        pbxNativeTargetGUID.Get() );

    // Footer
    m_Tmp += "</dict>\n"
             "</plist>\n";

    return m_Tmp;
}

// GenerateXCScheme
//------------------------------------------------------------------------------
const AString & XCodeProjectGenerator::GenerateXCScheme()
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    AStackString<> pbxLegacyTargetGUID;
    GetGUID_PBXLegacyTarget( 0, pbxLegacyTargetGUID );

    const AString & defaultConfigName = m_Configs[ 0 ]->m_Config;
    const char * documentVersioning = m_XCodeDocumentVersioning ? "YES" : "NO";

    // Macro Expansion
    AStackString<> macroExpansion;
    macroExpansion.Format( "      <MacroExpansion>\n"
                           "         <BuildableReference\n"
                           "            BuildableIdentifier = \"primary\"\n"
                           "            BlueprintIdentifier = \"%s\"\n"
                           "            BuildableName = \"%s\"\n"
                           "            BlueprintName = \"%s\"\n"
                           "            ReferencedContainer = \"container:%s.xcodeproj\">\n"
                           "         </BuildableReference>\n"
                           "      </MacroExpansion>\n",
                        pbxLegacyTargetGUID.Get(), m_ProjectName.Get(), m_ProjectName.Get(), m_ProjectName.Get() );

    // Header
    m_Tmp.AppendFormat( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<Scheme\n"
                        "   LastUpgradeVersion = \"1020\"\n"
                        "   version = \"1.3\">\n" );

    // Build Action
    m_Tmp.AppendFormat( "   <BuildAction\n"
                        "      parallelizeBuildables = \"YES\"\n"
                        "      buildImplicitDependencies = \"YES\">\n"
                        "      <BuildActionEntries>\n"
                        "         <BuildActionEntry\n"
                        "            buildForTesting = \"YES\"\n"
                        "            buildForRunning = \"YES\"\n"
                        "            buildForProfiling = \"YES\"\n"
                        "            buildForArchiving = \"YES\"\n"
                        "            buildForAnalyzing = \"YES\">\n"
                        "            <BuildableReference\n"
                        "               BuildableIdentifier = \"primary\"\n"
                        "               BlueprintIdentifier = \"%s\"\n"
                        "               BuildableName = \"%s\"\n"
                        "               BlueprintName = \"%s\"\n"
                        "               ReferencedContainer = \"container:%s.xcodeproj\">\n"
                        "            </BuildableReference>\n"
                        "         </BuildActionEntry>\n"
                        "      </BuildActionEntries>\n"
                        "   </BuildAction>\n",
                        pbxLegacyTargetGUID.Get(), m_ProjectName.Get(), m_ProjectName.Get(), m_ProjectName.Get() );                       

    // Test Action
    m_Tmp.AppendFormat( "   <TestAction\n"
                        "      buildConfiguration = \"%s\"\n"
                        "      selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"\n"
                        "      selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"\n"
                        "      shouldUseLaunchSchemeArgsEnv = \"YES\">\n"
                        "      <Testables>\n"
                        "      </Testables>\n"
                        "      <AdditionalOptions>\n"
                        "      </AdditionalOptions>\n"
                        "   </TestAction>\n",
                        defaultConfigName.Get() );

    // Launch Action
    m_Tmp.AppendFormat( "   <LaunchAction\n"
                        "      buildConfiguration = \"%s\"\n"
                        "      selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"\n"
                        "      selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"\n"
                        "      launchStyle = \"0\"\n"
                        "      useCustomWorkingDirectory = \"YES\"\n"
                        "      customWorkingDirectory = \"$(FASTBUILD_DEBUG_WORKING_DIR)\"\n"
                        "      ignoresPersistentStateOnLaunch = \"NO\"\n"
                        "      debugDocumentVersioning = \"%s\"\n"
                        "      debugServiceExtension = \"internal\"\n"
                        "      allowLocationSimulation = \"YES\">\n"
                        "%s",
                        defaultConfigName.Get(), documentVersioning, macroExpansion.Get() );
    if ( ( m_XCodeCommandLineArguments.IsEmpty() == false ) || ( m_XCodeCommandLineArgumentsDisabled.IsEmpty() == false ) )
    {
        m_Tmp.AppendFormat( "      <CommandLineArguments>\n" );
        for ( const AString & arg : m_XCodeCommandLineArguments )
        {
            AStackString<> escapedArgument;
            EscapeArgument( arg, escapedArgument );
            m_Tmp.AppendFormat( "         <CommandLineArgument\n"
                                "            argument = \"%s\"\n"
                                "            isEnabled = \"YES\">\n"
                                "         </CommandLineArgument>\n",
                                escapedArgument.Get() );
        }
        for ( const AString & arg : m_XCodeCommandLineArgumentsDisabled )
        {
            AStackString<> escapedArgument;
            EscapeArgument( arg, escapedArgument );
            m_Tmp.AppendFormat( "         <CommandLineArgument\n"
                                "            argument = \"%s\"\n"
                                "            isEnabled = \"NO\">\n"
                                "         </CommandLineArgument>\n",
                                escapedArgument.Get() );                            
        }        
        m_Tmp.AppendFormat( "      </CommandLineArguments>\n" );
    }
    m_Tmp.AppendFormat( "      <AdditionalOptions>\n"
                        "      </AdditionalOptions>\n"
                        "   </LaunchAction>\n" );

    // Profile Action
    m_Tmp.AppendFormat( "   <ProfileAction\n"
                        "      buildConfiguration = \"%s\"\n"
                        "      shouldUseLaunchSchemeArgsEnv = \"YES\"\n"
                        "      savedToolIdentifier = \"\"\n"
                        "      useCustomWorkingDirectory = \"NO\"\n"
                        "      debugDocumentVersioning = \"%s\">\n"
                        "%s"
                        "   </ProfileAction>\n",
                        defaultConfigName.Get(), documentVersioning, macroExpansion.Get() );

    // Analyze Action
    m_Tmp.AppendFormat( "   <AnalyzeAction\n"
                        "      buildConfiguration = \"%s\">\n"
                        "   </AnalyzeAction>\n",
                        defaultConfigName.Get() );

    // Archive Action
    m_Tmp.AppendFormat( "   <ArchiveAction\n"
                        "      buildConfiguration = \"%s\"\n"
                        "      revealArchiveInOrganizer = \"YES\">\n"
                        "   </ArchiveAction>\n",
                        defaultConfigName.Get() );

    // Footer
    m_Tmp.AppendFormat( "</Scheme>\n" );

    return m_Tmp;
}

void XCodeProjectGenerator::GetGUID_PBXGroup( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x22222222;
    const uint32_t part2 = 0x22222222;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_PBXLegacyTarget( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x33333333;
    const uint32_t part2 = 0x33333333;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_PBXNativeTarget( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x33333333;
    const uint32_t part2 = 0x00000000;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_PBXProject( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x44444444;
    const uint32_t part2 = 0x44444444;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_XConfigurationList( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x55555555;
    const uint32_t part2 = 0x55555555;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_XCBuildConfiguration( const uint32_t index, AString & outGUID )
{
    const uint32_t part1 = 0x66666666;
    const uint32_t part2 = 0x66666666;
    const uint32_t part3 = index;
    outGUID.Format( "%08X%08X%08X", part1, part2, part3 );
}

void XCodeProjectGenerator::GetGUID_PBXSourcesBuildPhase( const uint32_t index, AString & outGUID )
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
    // Files (PBXBuildFile)
    Write( "\n" );
    Write( "/* Begin PBXBuildFile section */\n" );
    for ( const File * file : m_Files )
    {
        // Get just the filename part from the full path
        const char * shortName = file->m_FileName.Get();

        const char * shortFolderName = "Sources";

        Write( "\t\t1111111100000000%08u /* %s in %s */ = {isa = PBXBuildFile; fileRef = 1111111111111111%08u /* %s */; };\n",
                    file->m_SortedIndex, shortName, shortFolderName, file->m_SortedIndex, shortName );
    }
    Write( "/* End PBXBuildFile section */\n" );

    // Files (PBXFileReference)
    Write( "\n" );
    Write( "/* Begin PBXFileReference section */\n" );
    for ( const File * file : m_Files )
    {
        // Get just the filename part from the full path
        const char * shortName = file->m_FileName.Get();

        // work out file type based on extension
        // TODO: What is the definitive list of these?
        const char * lastKnownFileType = "sourcecode.cpp.cpp";
        const char * fileEncoding = " fileEncoding = 4;";
        if ( file->m_FileName.EndsWithI( ".h" ) )
        {
            lastKnownFileType = "sourcecode.c.h";
        }
        else if ( file->m_FileName.EndsWithI( ".xcodeproj" ))
        {
            lastKnownFileType = "\"wrapper.pb-project\"";
            fileEncoding = "";
        }

        Write( "\t\t1111111111111111%08u /* %s */ = {isa = PBXFileReference;%s lastKnownFileType = %s; name = %s; path = %s; sourceTree = \"<group>\"; };\n",
                    file->m_SortedIndex, shortName, fileEncoding, lastKnownFileType, shortName, file->m_FullPath.Get() );
    }
    Write( "/* End PBXFileReference section */\n" );
}

// WriteFolders
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteFolders()
{
    // Folders
    Write( "\n" );
    Write( "/* Begin PBXGroup section */\n" );
    for ( const Folder * folder : m_Folders )
    {
        AStackString<> pbxGroupGUID;
        GetGUID_PBXGroup( folder->m_SortedIndex, pbxGroupGUID );

        const char * folderName = nullptr; // root folder is unnamed
        if ( folder->m_Path.IsEmpty() == false )
        {
            folderName = folder->m_Path.FindLast( NATIVE_SLASH );
            folderName = folderName ? ( folderName + 1 ) : folder->m_Path.Get();
        }

        if ( folderName )
        {
            Write( "\t\t%s /* %s */ = {\n"
                   "\t\t\tisa = PBXGroup;\n"
                   "\t\t\tchildren = (\n",
                   pbxGroupGUID.Get(), folderName );
        }
        else
        {
            Write( "\t\t%s = {\n"
                   "\t\t\tisa = PBXGroup;\n"
                   "\t\t\tchildren = (\n",
                   pbxGroupGUID.Get() );
        }
        
        // Child Files
        for ( const File * file : folder->m_Files )
        {
            Write( "\t\t\t\t1111111111111111%08u /* %s */,\n",
                   file->m_SortedIndex, file->m_FileName.Get() );
        }

        // Child Folders
        for ( const Folder * childFolder : folder->m_Folders )
        {
            const char * shortName = childFolder->m_Path.FindLast( NATIVE_SLASH );
            shortName = shortName ? ( shortName + 1 ) : childFolder->m_Path.Get();
            AStackString<> pbxGroupGUIDChild;
            GetGUID_PBXGroup( childFolder->m_SortedIndex, pbxGroupGUIDChild );
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
    AStackString<> buildPhaseGuid;
    GetGUID_PBXSourcesBuildPhase( 0, buildPhaseGuid );

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
        Write( "\t\t\t\t%s /* Sources */,\n", buildPhaseGuid.Get() );
        Write( "\t\t\t);\n" );
        WriteString( 3, "buildToolPath", m_XCodeBuildToolPath );
        WriteString( 3, "buildWorkingDirectory", m_XCodeBuildWorkingDir );
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
    Write( "\t\t\t\tLastUpgradeCheck = 1020;\n" );
    WriteString( 4, "ORGANIZATIONNAME", m_XCodeOrganizationName );
    Write( "\t\t\t\tTargetAttributes = {\n" );
    Write( "\t\t\t\t\t%s = {\n", pbxNativeTargetGUID.Get() );
    Write( "\t\t\t\t\t\tCreatedOnToolsVersion = 6.3.2;\n" );
    Write( "\t\t\t\t\t};\n" );
    Write( "\t\t\t\t\t%s = {\n", pbxLegacyTargetGUID.Get() );
    Write( "\t\t\t\t\t\tCreatedOnToolsVersion = 6.3.2;\n" );
    Write( "\t\t\t\t\t};\n" );
    Write( "\t\t\t\t};\n" );
    Write( "\t\t\t};\n" );
    Write( "\t\t\tbuildConfigurationList = %s /* Build configuration list for PBXProject \"%s\" */;\n", xConfigurationListGUID.Get(), m_ProjectName.Get() );
    Write( "\t\t\tcompatibilityVersion = \"Xcode 3.2\";\n" );
    Write( "\t\t\tdevelopmentRegion = en;\n" );
    Write( "\t\t\thasScannedForEncodings = 0;\n" );
    Write( "\t\t\tknownRegions = (\n" );
    Write( "\t\t\t\ten,\n" );
    Write( "\t\t\t\tBase,\n" );
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
           "\t\t\tbuildActionMask = 0;\n" );

    Write( "\t\t\tfiles = (\n" );
    for ( const File * file : m_Files )
    {
        const char * shortFolderName = "Sources";

        Write( "\t\t\t\t1111111100000000%08u /* %s in %s */,\n",
               file->m_SortedIndex, file->m_FileName.Get(), shortFolderName );
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
    const AStackString<> yesString( "YES" );

    Write( "\n" );
    Write( "/* Begin XCBuildConfiguration section */\n" );

    uint32_t configId = 0;
    for ( const ProjectGeneratorBaseConfig * baseConfig : m_Configs )
    {
        const XCodeProjectConfig & config = *( (const XCodeProjectConfig *)baseConfig );

        AStackString<> xcBuildConfigurationGUID;
        GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
        ++configId;

        const AString & target = config.m_TargetNode ? config.m_TargetNode->GetName() : AString::GetEmpty();

        // Use user specified working dir if available
        AStackString<> debugWorkingDir( config.m_XCodeDebugWorkingDir );
        if ( debugWorkingDir.IsEmpty() )
        {
            // Fall back to the executable working dir if we can find it
            const FileNode * debugTarget = ProjectGeneratorBase::FindExecutableDebugTarget( config.m_TargetNode );
            if ( debugTarget )
            {
                const AString & exeName = debugTarget->GetName();
                const char * lastSlash = exeName.FindLast( NATIVE_SLASH );
                if ( lastSlash )
                {
                    debugWorkingDir.Assign( exeName.Get(), lastSlash );
                }
            }
        }
        #if defined( __WINDOWS__ )
            debugWorkingDir.Replace( NATIVE_SLASH, OTHER_SLASH ); // Convert to OSX style
        #endif

        Write( "\t\t%s /* %s */ = {\n"
               "\t\t\tisa = XCBuildConfiguration;\n"
               "\t\t\tbuildSettings = {\n",
               xcBuildConfigurationGUID.Get(), config.m_Config.Get() );
        WriteString( 4, "CLANG_ANALYZER_LOCALIZABILITY_NONLOCALIZED", yesString );
        WriteString( 4, "ENABLE_TESTABILITY", yesString );
        WriteString( 4, "FASTBUILD_DEBUG_WORKING_DIR", debugWorkingDir );
        WriteString( 4, "FASTBUILD_TARGET", target );
        WriteString( 4, "ONLY_ACTIVE_ARCH", yesString );
        if ( config.m_XCodeBaseSDK.IsEmpty() == false )
        {
            WriteString( 4, "SDKROOT", config.m_XCodeBaseSDK );
        }
        if ( config.m_XCodeIphoneOSDeploymentTarget.IsEmpty() == false )
        {
            WriteString( 4, "IPHONEOS_DEPLOYMENT_TARGET", config.m_XCodeIphoneOSDeploymentTarget );
        }
        Write( "\t\t\t};\n"
               "\t\t\tname = %s;\n"
               "\t\t};\n",
               config.m_Config.Get() );
    }

    configId = 100;
    for ( const ProjectGeneratorBaseConfig * config : m_Configs )
    {
        AStackString<> xcBuildConfigurationGUID;
        GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
        ++configId;

        Write( "\t\t%s /* %s */ = {\n"
               "\t\t\tisa = XCBuildConfiguration;\n"
               "\t\t\tbuildSettings = {\n",
               xcBuildConfigurationGUID.Get(), config->m_Config.Get() );

        Write( "\t\t\t\tCLANG_ENABLE_OBJC_WEAK = YES;\n"
               "\t\t\t\tOTHER_CFLAGS = \"\";\n"
               "\t\t\t\tOTHER_LDFLAGS = \"\";\n"
               "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";\n"
               "\t\t\t};\n"
               "\t\t\tname = %s;\n"
               "\t\t};\n",
               config->m_Config.Get() );
    }

    configId = 200;
    for ( const ProjectGeneratorBaseConfig * config : m_Configs )
    {
        AStackString<> xcBuildConfigurationGUID;
        GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
        ++configId;

        Write( "\t\t%s /* %s */ = {\n"
               "\t\t\tisa = XCBuildConfiguration;\n"
               "\t\t\tbuildSettings = {\n",
               xcBuildConfigurationGUID.Get(), config->m_Config.Get() );

        Write( "\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;\n" );

        // TODO:B Can this (and other warning settings) be derived from the compiler options automatically?
        Write( "\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";\n" );

        // Find target from which to extract Intellisense options
        const ObjectListNode * oln = ProjectGeneratorBase::FindTargetForIntellisenseInfo( config->m_TargetNode );
        if ( oln )
        {
            // Defines
            {
                Array< AString > defines;
                ProjectGeneratorBase::ExtractDefines( oln->GetCompilerOptions(), defines, true );
                WriteArray( 4, "GCC_PREPROCESSOR_DEFINITIONS", defines );
            }

            // System Include Paths
            // TODO:B Do we need to differentiate include path types?
            {
                //Write( "\t\t\t\tHEADER_SEARCH_PATHS = (\n" );
                //Write( "%s", includePathsStr.Get() );
                //Write( "\t\t\t\t);\n" );
            }
        }

        Write( "\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";\n" );

        if ( oln )
        {
            // User Include Paths
            {
                Array< AString > includePaths;
                Array< AString > forceIncludePaths; // TODO:C Is there a place in XCode projects to put this?
                ProjectGeneratorBase::ExtractIncludePaths( oln->GetCompilerOptions(), includePaths, forceIncludePaths, true );
                for ( AString & include : includePaths )
                {
                    AStackString<> fullIncludePath;
                    NodeGraph::CleanPath( include, fullIncludePath ); // Expand to full path - TODO:C would be better to be project relative
                    include = fullIncludePath;
                    #if defined( __WINDOWS__ )
                        include.Replace( '\\', '/' ); // Convert to OSX style slashes
                    #endif
                }
                WriteArray( 4, "USER_HEADER_SEARCH_PATHS", includePaths );
            }
        }

        Write( "\t\t\t};\n"
               "\t\t\tname = %s;\n"
               "\t\t};\n",
               config->m_Config.Get() );
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
        for ( const ProjectGeneratorBaseConfig * config : m_Configs )
        {
            AStackString<> xcBuildConfigurationGUID;
            GetGUID_XCBuildConfiguration( configId, xcBuildConfigurationGUID );
            ++configId;

            Write( "\t\t\t\t%s /* %s */,\n", xcBuildConfigurationGUID.Get(), config->m_Config.Get() );
        }
        Write( "\t\t\t);\n" );
        Write( "\t\t\tdefaultConfigurationIsVisible = 0;\n" );
        Write( "\t\t\tdefaultConfigurationName = %s;\n", m_Configs[ 0 ]->m_Config.Get() ); // Use first config as default
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

// ShouldQuoteString
//------------------------------------------------------------------------------
bool XCodeProjectGenerator::ShouldQuoteString( const AString & value ) const
{
    if ( value.IsEmpty() )
    {
        return true;
    }
    for ( size_t i = 0; i < value.GetLength(); ++i )
    {
        const char c = value[ i ];
        if ( ( c == ' ' ) ||
             ( c == '"' ) ||
             ( c == '?' ) ||
             ( c == '-' ) ||
             ( c == '+' ) ||
             ( c == '=' ) )
        {
            return true;
        }
    }
    return false;
}

// WriteString
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteString( uint32_t indentDepth,
                                         const char * propertyName,
                                         const AString & value )
{
    // Prepare the indent string
    AStackString<> tabs;
    for ( uint32_t i = 0; i < indentDepth; ++i )
    {
        tabs += '\t';
    }
   
    // Empty strings and strings with spaces are quoted
    const char quoteString = ShouldQuoteString( value );
    const char * const formatString = quoteString ? "%s%s = \"%s\";\n"
                                                  : "%s%s = %s;\n";
    Write( formatString, tabs.Get(), propertyName, value.Get() );
}

// WriteArray
//------------------------------------------------------------------------------
void XCodeProjectGenerator::WriteArray( uint32_t indentDepth,
                                        const char * propertyName,
                                        const Array<AString> & values )
{
    // Prepare the indent string
    AStackString<> tabs;
    for ( uint32_t i = 0; i < indentDepth; ++i )
    {
        tabs += '\t';
    }

    // Handle empty or one item
    if ( values.GetSize() <= 1 )
    {
        // Empty arrays are written as a single empty string
        const AString & value = values.IsEmpty() ? AString::GetEmpty() : values[ 0 ];
        WriteString( indentDepth, propertyName, value );
        return;
    }

    // Write multi-element array
    Write( "%s%s = (\n", tabs.Get(), propertyName );
    for ( const AString & value : values )
    {
        // Empty strings and strings with spaces are quoted
        const char quoteString = ShouldQuoteString( value );
        const char * const formatString = quoteString ? "%s\t\"%s\",\n"
                                                      : "%s\t%s,\n";
        Write( formatString, tabs.Get(), value.Get() );
    }
    Write( "%s);\n", tabs.Get() );
}

// EscapeArgument
//------------------------------------------------------------------------------
void XCodeProjectGenerator::EscapeArgument( const AString & arg,
                                            AString & outEscapedArgument ) const
{
    for ( uint32_t i = 0; i < arg.GetLength(); ++i )
    {
        const char c = arg[ i ];
        switch ( c )
        {
            case '>': outEscapedArgument += "&gt;";     break;
            case '<': outEscapedArgument += "&lt;";     break;
            case '"': outEscapedArgument += "&quot;";   break;
            case '&': outEscapedArgument += "&amp;";    break;
            default:
            {
                outEscapedArgument += c;
            }
        }
    }
}

//------------------------------------------------------------------------------
