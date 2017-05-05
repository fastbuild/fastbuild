// VSProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "VSProjectGenerator.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h" // for GetEnvironmentString
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h" // TODO:C Remove when VSProjectGenerator derives from ProjectGeneratorBase

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args

// CONSTRUCTOR (VSProjectConfig)
//------------------------------------------------------------------------------
VSProjectConfig::VSProjectConfig()
    : m_Target( nullptr )
{
}

// DESTRUCTOR (VSProjectConfig)
//------------------------------------------------------------------------------
VSProjectConfig::~VSProjectConfig() = default;

// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectGenerator::VSProjectGenerator()
    : m_BasePaths( 0, true )
    , m_References( 0, true )
    , m_ProjectReferences( 0, true )
    , m_Files( 1024, true )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectGenerator::~VSProjectGenerator() = default;

// SetBasePaths
//------------------------------------------------------------------------------
void VSProjectGenerator::SetBasePaths( const Array< AString > & paths )
{
    m_BasePaths = paths;
}

// AddFile
//------------------------------------------------------------------------------
void VSProjectGenerator::AddFile( const AString & file )
{
    // ensure slash consistency which we rely on later
    AStackString<> fileCopy( file );
    fileCopy.Replace( FORWARD_SLASH, BACK_SLASH );

    ASSERT( !m_Files.Find( fileCopy ) );
    m_Files.Append( fileCopy );
}

// AddFiles
//------------------------------------------------------------------------------
void VSProjectGenerator::AddFiles( const Array< AString > & files )
{
    const AString * const fEnd = files.End();
    for ( const AString * fIt = files.Begin(); fIt!=fEnd; ++fIt )
    {
        AddFile( *fIt );
    }
}

// GetDeterministicProjectGUID
//------------------------------------------------------------------------------
/*static*/ void VSProjectGenerator::FormatDeterministicProjectGUID( AString & guid, const AString & projectName )
{
    // Replace native slash with Windows-style slash for GUID generation to keep GUIDs consistent across platforms
    AStackString<> projectNameNormalized( projectName );
    projectNameNormalized.Replace( NATIVE_SLASH, BACK_SLASH );
    guid.Format( "{%08x-6c94-4f93-bc2a-7f5284b7d434}", CRC32::Calc( projectNameNormalized ) );
}

// IsFilenameAHeaderFile
//------------------------------------------------------------------------------
static inline const bool IsFilenameAHeaderFile( const AString & fileName )
{
    // TODO: Add support for other header types?
    return ( fileName.EndsWith( 'h' ) || fileName.EndsWith( ".hpp" ) || fileName.EndsWith( ".hxx" ) || fileName.EndsWith( ".inl" ) );
}

static inline const bool ShouldWeSkipThisPlatform( const AString & platformIncludePattern, const AString & platform )
{
    // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, return true [So we can skip emitting this platform to the .vcxproj]
    const char *thisPlatformIncludePattern = platformIncludePattern.Get();
    ASSERT( (thisPlatformIncludePattern != nullptr) && (platformIncludePattern != "") );
    return ( (!platformIncludePattern.IsEmpty()) && (!platform.Find( thisPlatformIncludePattern )) );
}

static inline const char* GetPossiblyAliasedPlatformStringInUppercase( const AString & targetArchitectureAlias, const AString & platform )
{
    // Handle Platform Aliases - these define new platforms and alias existing platforms to them
    if( !targetArchitectureAlias.IsEmpty() )
    {
        const char *thisPlatformAlias = targetArchitectureAlias.Get();
        ASSERT( (thisPlatformAlias != nullptr) && (targetArchitectureAlias != "") );
        AStackString<>							thisPlatformAliasString( thisPlatformAlias );
                                                thisPlatformAliasString.ToUpper();
        const char *thisPlatformAliasUpper =	thisPlatformAliasString.Get();
        return thisPlatformAliasUpper;
    }
    return platform.Get();
}

static inline const char* GetPossiblyAliasedPlatformString( const AString & targetArchitectureAlias, const AString & platform )
{
    // Handle Platform Aliases - these define new platforms and alias existing platforms to them
    if( !targetArchitectureAlias.IsEmpty() )
    {
        ASSERT( (targetArchitectureAlias.Get() != nullptr) && (targetArchitectureAlias != "") );
        return targetArchitectureAlias.Get();
    }
    return platform.Get();
}

// GenerateVCXProj
//------------------------------------------------------------------------------
const AString & VSProjectGenerator::GenerateVCXProj( const AString & projectFile,
                                                     const Array< VSProjectConfig > & configs,
                                                     const Array< VSProjectFileType > & fileTypes )
{
    ASSERT( !m_ProjectName.IsEmpty() ); // needed for valid guid generation

    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = projectFile.FindLast( NATIVE_SLASH );
    AStackString<> projectBasePath( projectFile.Get(), lastSlash ? lastSlash + 1 : projectFile.Get() );

    const bool writeCSProj		= projectFile.EndsWithI( ".csproj" );
    const bool writeAndroidProj = projectFile.EndsWithI( ".androidproj" );

    // Don't write a custom build command by default, because as of Visual Studio 2015 R3, Makefile projects can't currently utilize this functionality
    // However, if we have a Utility project, then we can execute per-file custom build steps
    bool writeCustomBuildCommands = false;

    // header
    Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    Write( "<Project DefaultTargets=\"Build\" ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

    // Project Configurations
    {
        Write( "  <ItemGroup Label=\"ProjectConfigurations\">\n" );
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            const char *thisPlatform = GetPossiblyAliasedPlatformString( cIt->m_TargetArchitectureAlias, cIt->m_Platform );
            const char *thisConfiguration = cIt->m_Config.Get();

            // Handle Platform Aliases - these define new platforms and alias existing platforms to them
            if( !cIt->m_TargetArchitectureAlias.IsEmpty() )
            {
                const char *thisPlatformAliasUpper	= GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform );
                const char *thisPlatformAlias		= thisPlatform;

                Write( "    <ProjectConfiguration Include=\"%s|%s\">\n", thisConfiguration, thisPlatformAliasUpper );
                Write( "      <Configuration>%s</Configuration>\n", thisConfiguration );
                Write( "      <Platform>%s</Platform>\n", thisPlatformAlias );
                Write( "      <TargetArchitecture>%s</TargetArchitecture>\n", thisPlatformAlias );
                Write( "    </ProjectConfiguration>\n" );
            }

            Write( "    <ProjectConfiguration Include=\"%s|%s\">\n", thisConfiguration, cIt->m_Platform.Get() );	// We're explicitly using cIt->m_Platform.Get() here. We do not want to use thisPlatform, because it may be == thisPlatformAlias.
            Write( "      <Configuration>%s</Configuration>\n", thisConfiguration );
            Write( "      <Platform>%s</Platform>\n", thisPlatform );												// We're explicitly using thisPlatform (which may be == thisPlatformAlias) here.
            Write( "    </ProjectConfiguration>\n" );

            // Don't write a custom build command by default, because as of Visual Studio 2015 R3, Makefile projects can't currently utilize this functionality
            if ( !cIt->m_ProjectBuildType.IsEmpty( ) )
            {
                if ( cIt->m_ProjectBuildType == "Utility" )
                {
                    // Visual Studio Utility Projects Can Execute Per-File Custom Build Steps
                    writeCustomBuildCommands = true;
                }
            }

        }

        Write( "  </ItemGroup>\n" );
    }

    // files
    if( !writeAndroidProj ) // We don't need files in .androidproj, because VS ignores them anyway
    {
        const char * environment = FBuild::Get( ).GetEnvironmentString( );
        Write("  <ItemGroup>\n" );
        const AString * const fEnd = m_Files.End();
        for ( const AString * fIt = m_Files.Begin(); fIt!=fEnd; ++fIt )
        {
            const char * fileName = fIt->BeginsWithI( projectBasePath ) ? fIt->Get() + projectBasePath.GetLength() : fIt->Get();
            const char * fileType = nullptr;
            const VSProjectFileType * const end = fileTypes.End();
            for ( const VSProjectFileType * it=fileTypes.Begin(); it!=end; ++it )
            {
                if ( AString::MatchI( it->m_Pattern.Get(), fileName ) )
                {
                    fileType = it->m_FileType.Get();
                    break;
                }
            }

            const bool   isHeader = IsFilenameAHeaderFile( *fIt );
            const char * includeTypeString = isHeader ? "ClInclude" : "CustomBuild";
            Write( "    <%s Include=\"%s\">\n", includeTypeString, fileName );
            if ( fileType )
            {
                Write( "        <FileType>%s</FileType>\n", fileType );
            }

            if ( writeCustomBuildCommands && !isHeader )
            {
                // Build Command [Conditional]
                Write( "        <Message>Compiling %%(Identity)</Message>\n" );
                const VSProjectConfig * const cEnd = configs.End( );
                for ( const VSProjectConfig * cIt = configs.Begin( ); cIt != cEnd; ++cIt )
                {
                    // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
                    if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
                    {
                        continue;
                    }

                    const ObjectListNode * oln = ProjectGeneratorBase::FindTargetForIntellisenseInfo( cIt->m_Target );
                    if ( oln != nullptr )
                    {
                        AString my_compilerName( ( oln != nullptr ) ? oln->GetCompiler( )->GetName( ).Get( ) : "error: compiler unknown" );
                        AString my_compilerOptions( ( oln != nullptr ) ? oln->GetCompilerOptions( ).Get( ) : "error: compiler options unknown" );

                        my_compilerOptions.Replace( "%1", "%(FullPath)" );
                        my_compilerOptions.Replace( "%2", "$(OutDir)%(Identity).obj" );

                        Write( "        <Command Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">@echo on &amp; SET %s; &amp; %s %s</Command>\n", cIt->m_Config.Get( ), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ), environment, my_compilerName.Get( ), my_compilerOptions.Get( ) );
                    }
                    else
                    {
                        Write( "        <Command Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">echo FastBuild VSProjectGenerator.cpp Failed To Generate Command</Command>\n", cIt->m_Config.Get( ), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ) );
                    }
                }

                Write( "        <Outputs>$(OutDir)%%(Identity).obj</Outputs>\n" );
            }

            Write( "    </%s>\n", includeTypeString );
        }

        Write("  </ItemGroup>\n" );
    }

    // References
    {
        Write("  <ItemGroup>\n" );
        {
            // Project References
            const AString * const end = m_ProjectReferences.End();
            for ( const AString *  it = m_ProjectReferences.Begin(); it != end; ++it )
            {
                AStackString<> proj( *it );
                const char * pipe = proj.Find( '|' );
                if ( pipe )
                {
                    proj.SetLength( (uint32_t)( pipe - proj.Get() ) );
                    AStackString<> guid( pipe + 1 );
                    Write( "    <ProjectReference Include=\"%s\">\n", proj.Get() );
                    Write( "      <Project>%s</Project>\n", guid.Get() );
                    Write( "    </ProjectReference>\n" );
                }
                else
                {
                    Write( "    <ProjectReference Include=\"%s\" />\n", proj.Get() );
                }
            }
        }
        {
            // References
            const AString * const end = m_References.End();
            for ( const AString * it = m_References.Begin(); it != end; ++it )
            {
                Write( "    <Reference Include=\"%s\" />\n", it->Get() );
            }
        }
        Write("  </ItemGroup>\n" );
    }

    // GUID : use user provided one if available, otherwise generate one deterministically
    AStackString<> guid;
    if ( m_ProjectGuid.IsEmpty() )
    {
        FormatDeterministicProjectGUID( guid, m_ProjectName );
    }
    else
    {
        guid = m_ProjectGuid;
    }

    if ( writeAndroidProj )
    {
        // Globals
        Write( "  <PropertyGroup Label=\"Globals\">\n" );
        Write( "    <DisableReferencesFolder>true</DisableReferencesFolder>\n" );
        Write( "    <UseDefaultPropertyPageSchemas>false</UseDefaultPropertyPageSchemas>\n" );
        Write( "    <RootNamespace>Android11</RootNamespace>\n" );
        Write( "    <ProjectVersion>1.0</ProjectVersion>\n" );
        WritePGItem( "ProjectGuid", guid );
        Write( "    <ApkDebuggingProject>true</ApkDebuggingProject>\n" );
        Write( "    <DebuggerFlavor>AndroidDebugger</DebuggerFlavor>\n" );

        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            const char *thisConfig = cIt->m_Config.Get();
            const char *thisPlatform = GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform );

            Write( "    <PackagePath Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">%s</PackagePath>\n", thisConfig, thisPlatform, cIt->m_PackagePath.Get() );
            Write( "    <LaunchActivity Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">%s</LaunchActivity>\n", thisConfig, thisPlatform, cIt->m_LaunchActivity.Get() );
            Write( "    <AdditionalSymbolSearchPaths Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">%s</AdditionalSymbolSearchPaths>\n", thisConfig, thisPlatform, cIt->m_AdditionalSymbolSearchPaths.Get() );
            Write( "    <AdditionalSourceSearchPaths Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">%s</AdditionalSourceSearchPaths>\n", thisConfig, thisPlatform, cIt->m_AdditionalIncludePaths.Get() );
        }

        Write( "  </PropertyGroup>\n" );

        Write( "  <Import Project=\"$(AndroidTargetsPath)\\Android.Default.props\" />\n" );
        Write( "  <Import Project=\"$(AndroidTargetsPath)\\Android.props\" />\n" );
        Write( "  <Import Project=\"$(AndroidTargetsPath)\\Android.targets\" />\n" );

        Write( "  <Target Name=\"Build\" />\n" );
        Write( "  <Target Name=\"Clean\" />\n" );
        Write( "  <Target Name=\"Rebuild\" />\n" );

        Write( "  <ItemGroup>\n" );
        Write( "    <PropertyPageSchema Include=\"$(XamlDirectory)\\ProjectItemsSchema.xaml\" />\n" );
        Write( "    <PropertyPageSchema Include=\"$(XamlDirectory)\\AndroidDebugger.xaml\">\n" );
        Write( "      <Context>Project</Context>\n" );
        Write( "    </PropertyPageSchema>\n" );
        Write( "    <PropertyPageSchema Include=\"$(XamlDirectory)\\folder.xaml\">\n" );
        Write( "      <Context>File;BrowseObject</Context>\n" );
        Write( "    </PropertyPageSchema>\n" );
        Write( "    <PropertyPageSchema Include=\"$(XamlDirectory)\\DebuggerGeneral.xaml\" />\n" );
        Write( "  </ItemGroup>\n" );

        Write( "</Project>" ); // carriage return at end

        m_OutputVCXProj = m_Tmp;
        return m_OutputVCXProj;
    }

    // Globals
    Write( "  <PropertyGroup Label=\"Globals\">\n" );
    WritePGItem( "RootNamespace", m_RootNamespace );
    WritePGItem( "ProjectGuid", guid );
    WritePGItem( "DefaultLanguage", m_DefaultLanguage );
    WritePGItem( "Keyword", AStackString<>( "MakeFileProj" ) );
    WritePGItem( "ApplicationEnvironment", m_ApplicationEnvironment );
    Write( "  </PropertyGroup>\n" );

    // Default props
    Write( "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n" );

    // Configurations
    {
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            Write( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"Configuration\">\n", cIt->m_Config.Get(), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ) );

            if( !cIt->m_ProjectBuildType.IsEmpty( ) )
            {
                WritePGItem( "ConfigurationType", cIt->m_ProjectBuildType );
            }
            else
            {
                Write( "    <ConfigurationType>Makefile</ConfigurationType>\n" );
            }

            Write( "    <UseDebugLibraries>false</UseDebugLibraries>\n" );

            WritePGItem( "PlatformToolset",                 cIt->m_PlatformToolset );
            WritePGItem( "LocalDebuggerCommandArguments",   cIt->m_LocalDebuggerCommandArguments );
            WritePGItem( "LocalDebuggerCommand",            cIt->m_LocalDebuggerCommand );
            WritePGItem( "LocalDebuggerEnvironment",        cIt->m_LocalDebuggerEnvironment );
            WritePGItem( "WebBrowserDebuggerHttpUrl",		cIt->m_WebBrowserDebuggerHttpUrl );

            Write( "  </PropertyGroup>\n" );
        }
    }

    // Imports
    {
        Write( "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n" );
        Write( "  <ImportGroup Label=\"ExtensionSettings\">\n" );
        Write( "  </ImportGroup>\n" );
    }

    // Property Sheets
    {
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            Write( "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ) );
            Write( "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n" );
            Write( "  </ImportGroup>\n" );
        }
    }

    // User macros
    Write( "  <PropertyGroup Label=\"UserMacros\" />\n" );

    // Property Group
    {
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            Write( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ) );

            WritePGItem( "NMakeBuildCommandLine",           cIt->m_BuildCommand );
            WritePGItem( "NMakeReBuildCommandLine",         cIt->m_RebuildCommand );
            WritePGItem( "NMakeCleanCommandLine",           cIt->m_CleanCommand );
            WritePGItem( "NMakeOutput",                     cIt->m_Output );
            WritePGItem( "TargetName",                      cIt->m_TargetName );
            WritePGItem( "TargetExt",                       cIt->m_TargetExt );

            AStackString<> vsIncludeSearchPath;
            if ( !cIt->m_AdditionalIncludePaths.IsEmpty( ) )
            {
                vsIncludeSearchPath = cIt->m_AdditionalIncludePaths;
                vsIncludeSearchPath += "$(IncludePath);";
                WritePGItem( "IncludePath", vsIncludeSearchPath );
            }

            const ObjectListNode * oln = nullptr;
            if ( cIt->m_PreprocessorDefinitions.IsEmpty() || cIt->m_IncludeSearchPath.IsEmpty() )
            {
                oln = ProjectGeneratorBase::FindTargetForIntellisenseInfo( cIt->m_Target );
            }

            if ( cIt->m_PreprocessorDefinitions.IsEmpty() == false )
            {
                WritePGItem( "NMakePreprocessorDefinitions",    cIt->m_PreprocessorDefinitions );
            }
            else
            {
                if ( oln )
                {
                    Array< AString > defines;
                    ProjectGeneratorBase::ExtractIntellisenseOptions( oln->GetCompilerOptions(), "/D", "-D", defines, false );
                    AStackString<> definesStr;
                    ProjectGeneratorBase::ConcatIntellisenseOptions( defines, definesStr, nullptr, ";" );
                    WritePGItem( "NMakePreprocessorDefinitions", definesStr );
                }
            }
            if ( cIt->m_IncludeSearchPath.IsEmpty() == false )
            {
                WritePGItem( "NMakeIncludeSearchPath",          cIt->m_IncludeSearchPath );
            }
            else
            {
                if ( oln )
                {
                    Array< AString > includePaths;
                    ProjectGeneratorBase::ExtractIntellisenseOptions( oln->GetCompilerOptions(), "/I", "-I", includePaths, false );
                    for ( AString & include : includePaths )
                    {
                        AStackString<> fullIncludePath;
                        NodeGraph::CleanPath( include, fullIncludePath ); // Expand to full path - TODO:C would be better to be project relative
                        include = fullIncludePath;
                    }
                    AStackString<> includePathsStr;
                    ProjectGeneratorBase::ConcatIntellisenseOptions( includePaths, includePathsStr, nullptr, ";" );
                    WritePGItem( "NMakeIncludeSearchPath", includePathsStr );
                }
            }
            WritePGItem( "NMakeForcedIncludes",             cIt->m_ForcedIncludes );
            WritePGItem( "NMakeAssemblySearchPath",         cIt->m_AssemblySearchPath );
            WritePGItem( "NMakeForcedUsingAssemblies",      cIt->m_ForcedUsingAssemblies );
            WritePGItem( "AdditionalOptions",               cIt->m_AdditionalOptions );
            WritePGItem( "Xbox360DebuggerCommand",          cIt->m_Xbox360DebuggerCommand );
            WritePGItem( "DebuggerFlavor",                  cIt->m_DebuggerFlavor );
            WritePGItem( "AumidOverride",                   cIt->m_AumidOverride );
            WritePGItem( "LocalDebuggerWorkingDirectory",   cIt->m_LocalDebuggerWorkingDirectory );
            WritePGItem( "IntDir",                          cIt->m_IntermediateDirectory );
            WritePGItem( "OutDir",                          cIt->m_OutputDirectory );
            WritePGItem( "LayoutDir",                       cIt->m_LayoutDir );
            WritePGItem( "LayoutExtensionFilter",           cIt->m_LayoutExtensionFilter );
            Write( "  </PropertyGroup>\n" );
        }
    }

    // ItemDefinition Groups
    {
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            // If we have a .PlatformIncludePattern filter defined, and this platform doesn't match it, then skip emitting this platform to the .vcxproj
            if( ShouldWeSkipThisPlatform( cIt->m_PlatformIncludePattern, cIt->m_Platform ) )
            {
                continue;
            }

            Write( "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), GetPossiblyAliasedPlatformStringInUppercase( cIt->m_TargetArchitectureAlias, cIt->m_Platform ) );
            Write( "    <BuildLog>\n" );
            Write( "      <Path />\n" );
            Write( "    </BuildLog>\n" );
            if ( ( !cIt->m_DeploymentType.IsEmpty() ) || ( !cIt->m_DeploymentFiles.IsEmpty() ) )
            {
                Write( "    <Deploy>\n" );
                WritePGItem( "DeploymentType",          cIt->m_DeploymentType );
                WritePGItem( "DeploymentFiles",         cIt->m_DeploymentFiles );
                Write( "    </Deploy>\n" );
            }
            Write( "  </ItemDefinitionGroup>\n" );
        }
    }

    // footer
    Write("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n" );
    Write("  <ImportGroup Label=\"ExtensionTargets\">\n" );
    Write("  </ImportGroup>\n" );
    Write("  <Import Condition=\"'$(ConfigurationType)' == 'Makefile' and Exists('$(VCTargetsPath)\\Platforms\\$(Platform)\\SCE.Makefile.$(Platform).targets')\" Project=\"$(VCTargetsPath)\\Platforms\\$(Platform)\\SCE.Makefile.$(Platform).targets\" />\n");
    Write( "</Project>" ); // carriage return at end

    m_OutputVCXProj = m_Tmp;
    return m_OutputVCXProj;
}

// GenerateVCXProjFilters
//------------------------------------------------------------------------------
const AString & VSProjectGenerator::GenerateVCXProjFilters( const AString & projectFile )
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    // determine folder for project
    const char * lastProjSlash = projectFile.FindLast( NATIVE_SLASH );
    AStackString<> projectBasePath( projectFile.Get(), lastProjSlash ? lastProjSlash + 1 : projectFile.Get() );

    // header
    Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    Write( "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

    // list of all folders
    Array< AString > folders( 1024, true );

    // files
    {
        Write( "  <ItemGroup>\n" );
        const AString * const fEnd = m_Files.End();
        for ( const AString * fIt = m_Files.Begin(); fIt!=fEnd; ++fIt )
        {
            // get folder part, relative to base dir
            AStackString<> folder;
            GetFolderPath( *fIt, folder );
            const char * fileName = fIt->BeginsWithI( projectBasePath ) ? fIt->Get() + projectBasePath.GetLength() : fIt->Get();
            const char * includeTypeString = IsFilenameAHeaderFile( *fIt ) ? "ClInclude" : "CustomBuild";

            Write( "    <%s Include=\"%s\">\n", includeTypeString, fileName );
            if ( !folder.IsEmpty() )
            {
                Write( "      <Filter>%s</Filter>\n", folder.Get() );
            }
            Write( "    </%s>\n", includeTypeString );

            // add new folders
            if ( !folder.IsEmpty() )
            {
                for (;;)
                {
                    // add this folder if unique
                    bool found = false;
                    for ( const AString * it=folders.Begin(); it!=folders.End(); ++it )
                    {
                        if ( it->CompareI( folder ) == 0 )
                        {
                            found = true;
                            break;
                        }
                    }
                    if ( !found )
                    {
                        folders.Append( folder );
                    }

                    // handle intermediate folders
                    const char * lastSlash = folder.FindLast( BACK_SLASH );
                    if ( lastSlash == nullptr )
                    {
                        break;
                    }
                    folder.SetLength( (uint32_t)( lastSlash - folder.Get() ) );
                }
            }
        }
        Write( "  </ItemGroup>\n" );
    }

    // folders
    {
        const AString * const fEnd = folders.End();
        for ( const AString * fIt = folders.Begin(); fIt!=fEnd; ++fIt )
        {
            Write( "  <ItemGroup>\n" );
            Write( "    <Filter Include=\"%s\">\n", fIt->Get() );
            Write( "      <UniqueIdentifier>{%08x-6c94-4f93-bc2a-7f5284b7d434}</UniqueIdentifier>\n", CRC32::Calc( *fIt ) );
            Write( "    </Filter>\n" );
            Write( "  </ItemGroup>\n" );
        }
    }

    // footer
    Write( "</Project>" ); // no carriage return

    m_OutputVCXProjFilters = m_Tmp;
    return m_OutputVCXProjFilters;
}

// Write
//------------------------------------------------------------------------------
void VSProjectGenerator::Write( const char * fmtString, ... )
{
    AStackString< 1024 > tmp;

    va_list args;
    va_start(args, fmtString);
    tmp.VFormat( fmtString, args );
    va_end( args );

    // resize output buffer in large chunks to prevent re-sizing
    if ( m_Tmp.GetLength() + tmp.GetLength() > m_Tmp.GetReserved() )
    {
        m_Tmp.SetReserved( m_Tmp.GetReserved() + MEGABYTE );
    }

    m_Tmp += tmp;
}

// WritePGItem
//------------------------------------------------------------------------------
void VSProjectGenerator::WritePGItem( const char * xmlTag, const AString & value )
{
    if ( value.IsEmpty() )
    {
        return;
    }
    Write( "    <%s>%s</%s>\n", xmlTag, value.Get(), xmlTag );
}

// GetFolderPath
//------------------------------------------------------------------------------
void VSProjectGenerator::GetFolderPath( const AString & fileName, AString & folder ) const
{
    const AString * const bEnd = m_BasePaths.End();
    for ( const AString * bIt = m_BasePaths.Begin(); bIt != bEnd; ++bIt )
    {
        const AString & basePath = *bIt;
        if ( fileName.BeginsWithI( basePath ) )
        {
            const char * begin = fileName.Get() + basePath.GetLength();
            const char * end = fileName.GetEnd();
            const char * lastSlash = fileName.FindLast( BACK_SLASH );
            end = ( lastSlash ) ? lastSlash : end;
            if ( begin < end )
            {
                folder.Assign( begin, end );
                return;
            }
        }
    }

    // no matching base path (use root)
    folder.Clear();
}

// VSProjectConfig::Save
//------------------------------------------------------------------------------
/*static*/ void VSProjectConfig::Save( IOStream & stream, const Array< VSProjectConfig > & configs )
{
    uint32_t numConfigs = (uint32_t)configs.GetSize();
    stream.Write( numConfigs );
    for ( uint32_t i=0; i<numConfigs; ++i )
    {
        const VSProjectConfig & cfg = configs[ i ];

        stream.Write( cfg.m_SolutionPlatform );
        stream.Write( cfg.m_SolutionConfig );

        stream.Write( cfg.m_Platform );
        stream.Write( cfg.m_Config );

        Node::SaveNodeLink( stream, cfg.m_Target );

        stream.Write( cfg.m_BuildCommand );
        stream.Write( cfg.m_RebuildCommand );
        stream.Write( cfg.m_CleanCommand );

        stream.Write( cfg.m_TargetName );
        stream.Write( cfg.m_TargetExt );

        stream.Write( cfg.m_Output );
        stream.Write( cfg.m_AdditionalIncludePaths );
        stream.Write( cfg.m_PreprocessorDefinitions );
        stream.Write( cfg.m_IncludeSearchPath );
        stream.Write( cfg.m_ForcedIncludes );
        stream.Write( cfg.m_AssemblySearchPath );
        stream.Write( cfg.m_ForcedUsingAssemblies );
        stream.Write( cfg.m_AdditionalOptions );
        stream.Write( cfg.m_OutputDirectory );
        stream.Write( cfg.m_IntermediateDirectory );
        stream.Write( cfg.m_LayoutDir );
        stream.Write( cfg.m_LayoutExtensionFilter );
        stream.Write( cfg.m_Xbox360DebuggerCommand );
        stream.Write( cfg.m_DebuggerFlavor );
        stream.Write( cfg.m_AumidOverride );
        stream.Write( cfg.m_PlatformToolset );
        stream.Write( cfg.m_DeploymentType );
        stream.Write( cfg.m_DeploymentFiles );
        stream.Write( cfg.m_PackagePath );
        stream.Write( cfg.m_LaunchActivity );
        stream.Write( cfg.m_PlatformIncludePattern );
        stream.Write( cfg.m_TargetArchitectureAlias );
        stream.Write( cfg.m_AdditionalSymbolSearchPaths );

        stream.Write( cfg.m_LocalDebuggerCommandArguments );
        stream.Write( cfg.m_LocalDebuggerWorkingDirectory );
        stream.Write( cfg.m_LocalDebuggerCommand );
        stream.Write( cfg.m_LocalDebuggerEnvironment );
        stream.Write( cfg.m_WebBrowserDebuggerHttpUrl );
        stream.Write( cfg.m_ProjectBuildType );

    }
}

// VSProjectConfig::Load
//------------------------------------------------------------------------------
/*static*/ bool VSProjectConfig::Load( NodeGraph & nodeGraph, IOStream & stream, Array< VSProjectConfig > & configs )
{
    ASSERT( configs.IsEmpty() );

    uint32_t numConfigs( 0 );
    if ( !stream.Read( numConfigs ) )
    {
        return false;
    }
    configs.SetSize( numConfigs );
    for ( uint32_t i=0; i<numConfigs; ++i )
    {
        VSProjectConfig & cfg = configs[ i ];

        if ( stream.Read( cfg.m_SolutionPlatform ) == false ) { return false; }
        if ( stream.Read( cfg.m_SolutionConfig ) == false ) { return false;  }

        if ( stream.Read( cfg.m_Platform ) == false ) { return false; }
        if ( stream.Read( cfg.m_Config ) == false ) { return false; }

        if ( !Node::LoadNodeLink( nodeGraph, stream, cfg.m_Target ) ) { return false; }

        if ( stream.Read( cfg.m_BuildCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_RebuildCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_CleanCommand ) == false ) { return false; }

        if ( stream.Read( cfg.m_TargetName ) == false ) { return false; }
        if ( stream.Read( cfg.m_TargetExt ) == false ) { return false; }

        if ( stream.Read( cfg.m_Output ) == false ) { return false; }
        if ( stream.Read( cfg.m_AdditionalIncludePaths ) == false ) { return false; }
        if ( stream.Read( cfg.m_PreprocessorDefinitions ) == false ) { return false; }
        if ( stream.Read( cfg.m_IncludeSearchPath ) == false ) { return false; }
        if ( stream.Read( cfg.m_ForcedIncludes ) == false ) { return false; }
        if ( stream.Read( cfg.m_AssemblySearchPath ) == false ) { return false; }
        if ( stream.Read( cfg.m_ForcedUsingAssemblies ) == false ) { return false; }
        if ( stream.Read( cfg.m_AdditionalOptions ) == false ) { return false; }
        if ( stream.Read( cfg.m_OutputDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_IntermediateDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_LayoutDir ) == false ) { return false; }
        if ( stream.Read( cfg.m_LayoutExtensionFilter ) == false ) { return false; }
        if ( stream.Read( cfg.m_Xbox360DebuggerCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_DebuggerFlavor ) == false ) { return false; }
        if ( stream.Read( cfg.m_AumidOverride ) == false ) { return false; }
        if ( stream.Read( cfg.m_PlatformToolset ) == false ) { return false; }
        if ( stream.Read( cfg.m_DeploymentType ) == false ) { return false; }
        if ( stream.Read( cfg.m_DeploymentFiles ) == false ) { return false; }
        if ( stream.Read( cfg.m_PackagePath ) == false ) { return false; }
        if ( stream.Read( cfg.m_LaunchActivity ) == false ) { return false; }
        if ( stream.Read( cfg.m_PlatformIncludePattern) == false ) { return false; }
        if ( stream.Read( cfg.m_TargetArchitectureAlias ) == false ) { return false; }
        if ( stream.Read( cfg.m_AdditionalSymbolSearchPaths ) == false ) { return false; }

        if ( stream.Read( cfg.m_LocalDebuggerCommandArguments ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerWorkingDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerEnvironment ) == false ) { return false; }
        if ( stream.Read( cfg.m_WebBrowserDebuggerHttpUrl ) == false ) { return false; }

        if ( stream.Read( cfg.m_ProjectBuildType ) == false ) { return false; }

    }
    return true;
}

// VSProjectFileType::Save
//------------------------------------------------------------------------------
/*static*/ void VSProjectFileType::Save( IOStream & stream, const Array< VSProjectFileType > & fileTypes )
{
    uint32_t numFileTypes = (uint32_t)fileTypes.GetSize();
    stream.Write( numFileTypes );
    for ( uint32_t i=0; i<numFileTypes; ++i )
    {
        const VSProjectFileType & ft = fileTypes[ i ];

        stream.Write( ft.m_FileType );
        stream.Write( ft.m_Pattern );
    }
}

// VSProjectFileType::Load
//------------------------------------------------------------------------------
/*static*/ bool VSProjectFileType::Load( IOStream & stream, Array< VSProjectFileType > & fileTypes )
{
    ASSERT( fileTypes.IsEmpty() );

    uint32_t numFileTypes( 0 );
    if ( !stream.Read( numFileTypes ) )
    {
        return false;
    }
    fileTypes.SetSize( numFileTypes );
    for ( uint32_t i=0; i<numFileTypes; ++i )
    {
        VSProjectFileType & ft = fileTypes[ i ];

        if ( stream.Read( ft.m_FileType ) == false ) { return false; }
        if ( stream.Read( ft.m_Pattern ) == false ) { return false; }
    }
    return true;
}

//------------------------------------------------------------------------------
