// VSProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "VSProjectGenerator.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
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
    , m_ProjectSccEntrySAK( false )
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

    // header
    Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    Write( "<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

    // Project Configurations
    {
        Write( "  <ItemGroup Label=\"ProjectConfigurations\">\n" );
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            Write( "    <ProjectConfiguration Include=\"%s|%s\">\n", cIt->m_Config.Get(), cIt->m_Platform.Get() );
            Write( "      <Configuration>%s</Configuration>\n", cIt->m_Config.Get() );
            Write( "      <Platform>%s</Platform>\n", cIt->m_Platform.Get() );
            Write( "    </ProjectConfiguration>\n" );
        }
        Write( "  </ItemGroup>\n" );
    }

    // files
    {
        Write("  <ItemGroup>\n" );
        const AString * const fEnd = m_Files.End();
        for ( const AString * fIt = m_Files.Begin(); fIt!=fEnd; ++fIt )
        {
            AStackString<> fileName;
            GetProjectRelativePath( projectBasePath, *fIt, fileName );
            const char * fileType = nullptr;
            const VSProjectFileType * const end = fileTypes.End();
            for ( const VSProjectFileType * it=fileTypes.Begin(); it!=end; ++it )
            {
                if ( AString::MatchI( it->m_Pattern.Get(), fileName.Get() ) )
                {
                    fileType = it->m_FileType.Get();
                    break;
                }
            }
            if ( fileType )
            {
                Write( "    <CustomBuild Include=\"%s\">\n", fileName.Get() );
                Write( "        <FileType>%s</FileType>\n", fileType );
                Write( "    </CustomBuild>\n" );
            }
            else
            {
                Write( "    <CustomBuild Include=\"%s\" />\n", fileName.Get() );
            }
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

    // Globals
    Write( "  <PropertyGroup Label=\"Globals\">\n" );
    WritePGItem( "RootNamespace", m_RootNamespace );
    WritePGItem( "ProjectGuid", guid );
    WritePGItem( "DefaultLanguage", m_DefaultLanguage );
    WritePGItem( "Keyword", AStackString<>( "MakeFileProj" ) );
    if ( m_ProjectSccEntrySAK )
    {
        const AStackString<> sakString( "SAK" );
        WritePGItem( "SccProjectName", sakString );
        WritePGItem( "SccAuxPath", sakString );
        WritePGItem( "SccLocalPath", sakString );
        WritePGItem( "SccProvider", sakString );
    }
    WritePGItem( "ApplicationEnvironment", m_ApplicationEnvironment );
    Write( "  </PropertyGroup>\n" );

    // Default props
    Write( "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n" );

    // Configurations
    {
        const VSProjectConfig * const cEnd = configs.End();
        for ( const VSProjectConfig * cIt = configs.Begin(); cIt!=cEnd; ++cIt )
        {
            Write( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"Configuration\">\n", cIt->m_Config.Get(), cIt->m_Platform.Get() );
            Write( "    <ConfigurationType>Makefile</ConfigurationType>\n" );
            Write( "    <UseDebugLibraries>false</UseDebugLibraries>\n" );

            WritePGItem( "PlatformToolset",                 cIt->m_PlatformToolset );
            WritePGItem( "LocalDebuggerCommandArguments",   cIt->m_LocalDebuggerCommandArguments );
            WritePGItem( "LocalDebuggerCommand",            cIt->m_LocalDebuggerCommand );
            WritePGItem( "LocalDebuggerEnvironment",        cIt->m_LocalDebuggerEnvironment );

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
            Write( "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), cIt->m_Platform.Get() );
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
            Write( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), cIt->m_Platform.Get() );

            WritePGItem( "NMakeBuildCommandLine",           cIt->m_BuildCommand );
            WritePGItem( "NMakeReBuildCommandLine",         cIt->m_RebuildCommand );
            WritePGItem( "NMakeCleanCommandLine",           cIt->m_CleanCommand );
            WritePGItem( "NMakeOutput",                     cIt->m_Output );

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
                        NodeGraph::CleanPath( include, fullIncludePath ); // Expand to full path
                        GetProjectRelativePath( projectBasePath, fullIncludePath, include );
                        #if !defined( __WINDOWS__ )
                            include.Replace( '/', '\\' ); // Convert to Windows-style slashes
                        #endif
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
            Write( "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", cIt->m_Config.Get(), cIt->m_Platform.Get() );
            Write( "    <BuildLog>\n" );
            if ( !cIt->m_BuildLogFile.IsEmpty() )
            {
                WritePGItem( "Path",          cIt->m_BuildLogFile );
            }
            else
            {
                Write( "      <Path />\n" );
            }
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
            AStackString<> fileName;
            GetProjectRelativePath( projectBasePath, *fIt, fileName );
            Write( "    <CustomBuild Include=\"%s\">\n", fileName.Get() );
            if ( !folder.IsEmpty() )
            {
                Write( "      <Filter>%s</Filter>\n", folder.Get() );
            }
            Write( "    </CustomBuild>\n" );

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

// GetProjectRelativePath
//------------------------------------------------------------------------------
/*static*/ void VSProjectGenerator::GetProjectRelativePath( const AString & projectFolderPath,
                                                            const AString & fileName,
                                                            AString & outRelativeFileName )
{
    // Find common sub-path
    const char * pathA = projectFolderPath.Get();
    const char * pathB = fileName.Get();
    while ( ( *pathA == *pathB ) && ( *pathA != '\0' ) )
    {
        pathA++;
        pathB++;
    }
    const bool hasCommonSubPath = ( pathA != projectFolderPath.Get() );
    if ( hasCommonSubPath == false )
    {
        // No common sub-path, so use relative name
        outRelativeFileName = fileName;
        return;
    }

    // Build relative path

    // For every remaining dir in the project path, go up one directory
    outRelativeFileName.Clear();
    for (;;)
    {
        const char c = *pathA;
        if ( c == 0 )
        {
            break;
        }
        if ( ( c == '/' ) || ( c == '\\' ) )
        {
            outRelativeFileName += "..\\";
        }
        ++pathA;
    }

    // Add remainder of source path relative to the common sub path
    outRelativeFileName += pathB;
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

        stream.Write( cfg.m_Output );
        stream.Write( cfg.m_PreprocessorDefinitions );
        stream.Write( cfg.m_IncludeSearchPath );
        stream.Write( cfg.m_ForcedIncludes );
        stream.Write( cfg.m_AssemblySearchPath );
        stream.Write( cfg.m_ForcedUsingAssemblies );
        stream.Write( cfg.m_AdditionalOptions );
        stream.Write( cfg.m_OutputDirectory );
        stream.Write( cfg.m_IntermediateDirectory );
        stream.Write( cfg.m_BuildLogFile );
        stream.Write( cfg.m_LayoutDir );
        stream.Write( cfg.m_LayoutExtensionFilter );
        stream.Write( cfg.m_Xbox360DebuggerCommand );
        stream.Write( cfg.m_DebuggerFlavor );
        stream.Write( cfg.m_AumidOverride );
        stream.Write( cfg.m_PlatformToolset );
        stream.Write( cfg.m_DeploymentType );
        stream.Write( cfg.m_DeploymentFiles );

        stream.Write( cfg.m_LocalDebuggerCommandArguments );
        stream.Write( cfg.m_LocalDebuggerWorkingDirectory );
        stream.Write( cfg.m_LocalDebuggerCommand );
        stream.Write( cfg.m_LocalDebuggerEnvironment );
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

        if ( stream.Read( cfg.m_Output ) == false ) { return false; }
        if ( stream.Read( cfg.m_PreprocessorDefinitions ) == false ) { return false; }
        if ( stream.Read( cfg.m_IncludeSearchPath ) == false ) { return false; }
        if ( stream.Read( cfg.m_ForcedIncludes ) == false ) { return false; }
        if ( stream.Read( cfg.m_AssemblySearchPath ) == false ) { return false; }
        if ( stream.Read( cfg.m_ForcedUsingAssemblies ) == false ) { return false; }
        if ( stream.Read( cfg.m_AdditionalOptions ) == false ) { return false; }
        if ( stream.Read( cfg.m_OutputDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_IntermediateDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_BuildLogFile ) == false ) { return false; }
        if ( stream.Read( cfg.m_LayoutDir ) == false ) { return false; }
        if ( stream.Read( cfg.m_LayoutExtensionFilter ) == false ) { return false; }
        if ( stream.Read( cfg.m_Xbox360DebuggerCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_DebuggerFlavor ) == false ) { return false; }
        if ( stream.Read( cfg.m_AumidOverride ) == false ) { return false; }
        if ( stream.Read( cfg.m_PlatformToolset ) == false ) { return false; }
        if ( stream.Read( cfg.m_DeploymentType ) == false ) { return false; }
        if ( stream.Read( cfg.m_DeploymentFiles ) == false ) { return false; }

        if ( stream.Read( cfg.m_LocalDebuggerCommandArguments ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerWorkingDirectory ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerCommand ) == false ) { return false; }
        if ( stream.Read( cfg.m_LocalDebuggerEnvironment ) == false ) { return false; }
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
