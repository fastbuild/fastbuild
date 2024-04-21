// VSProjectGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectGenerator.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h" // TODO:C Remove when VSProjectGenerator derives from ProjectGeneratorBase

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args

// FileAscendingCompareIDeref
//------------------------------------------------------------------------------
class FileAscendingCompareIDeref
{
public:
    inline bool operator () ( const VSProjectFilePair * a, const VSProjectFilePair * b ) const
    {
        return ( a->m_ProjectRelativePath.CompareI( b->m_ProjectRelativePath ) < 0 );
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectGenerator::VSProjectGenerator()
    : m_BasePaths( 0 )
    , m_ProjectSccEntrySAK( false )
    , m_References( 0 )
    , m_ProjectReferences( 0 )
    , m_FilePathsCanonicalized( false )
    , m_Files( 1024 )
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectGenerator::~VSProjectGenerator() = default;

// SetBasePaths
//------------------------------------------------------------------------------
void VSProjectGenerator::SetBasePaths( const Array< AString > & paths )
{
    ASSERT( m_FilePathsCanonicalized == false );
    m_BasePaths = paths;
}

// AddFile
//------------------------------------------------------------------------------
void VSProjectGenerator::AddFile( const AString & file )
{
    // ensure slash consistency which we rely on later
    AStackString<> fileCopy( file );
    fileCopy.Replace( FORWARD_SLASH, BACK_SLASH );
    m_Files.EmplaceBack();
    m_Files.Top().m_AbsolutePath = fileCopy;
}

// AddFiles
//------------------------------------------------------------------------------
void VSProjectGenerator::AddFiles( const Array< AString > & files )
{
    for ( const AString & file : files )
    {
        AddFile( file );
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
                                                     const Array< VSProjectFileType > & fileTypes,
                                                     const Array< VSProjectImport > & projectImports )
{
    ASSERT( !m_ProjectGuid.IsEmpty() );

    m_Tmp.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = projectFile.FindLast( NATIVE_SLASH );
    AStackString<> projectBasePath( projectFile.Get(), lastSlash ? lastSlash + 1 : projectFile.Get() );

    // Canonicalize and de-duplicate files
    CanonicalizeFilePaths( projectBasePath );

    // header
    Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    Write( "<Project DefaultTargets=\"Build\" ToolsVersion=\"15.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

    // Project Configurations
    {
        Write( "  <ItemGroup Label=\"ProjectConfigurations\">\n" );
        for ( const VSProjectConfig & config : configs )
        {
            WriteF("    <ProjectConfiguration Include=\"%s|%s\">\n", config.m_Config.Get(), config.m_Platform.Get() );
            WriteF("      <Configuration>%s</Configuration>\n", config.m_Config.Get() );
            WriteF("      <Platform>%s</Platform>\n", config.m_Platform.Get() );
            Write( "    </ProjectConfiguration>\n" );
        }
        Write( "  </ItemGroup>\n" );
    }

    // files
    {
        Write("  <ItemGroup>\n" );
        for ( const VSProjectFilePair & filePathPair : m_Files )
        {
            const AString & fileName = filePathPair.m_ProjectRelativePath;

            const char * fileTypeToUse = nullptr;
            for ( const VSProjectFileType & fileType : fileTypes )
            {
                if ( AString::MatchI( fileType.m_Pattern.Get(), fileName.Get() ) )
                {
                    fileTypeToUse = fileType.m_FileType.Get();
                    break;
                }
            }
            if ( fileTypeToUse )
            {
                WriteF( "    <CustomBuild Include=\"%s\">\n", fileName.Get() );
                WriteF( "        <FileType>%s</FileType>\n", fileTypeToUse );
                Write( "    </CustomBuild>\n" );
            }
            else
            {
                WriteF( "    <CustomBuild Include=\"%s\" />\n", fileName.Get() );
            }
        }
        Write("  </ItemGroup>\n" );
    }

    // References
    {
        Write("  <ItemGroup>\n" );
        {
            // Project References
            for ( const AString & projectReference : m_ProjectReferences )
            {
                AStackString<> proj( projectReference );
                const char * pipe = proj.Find( '|' );
                if ( pipe )
                {
                    proj.SetLength( (uint32_t)( pipe - proj.Get() ) );
                    AStackString<> guid( pipe + 1 );
                    WriteF("    <ProjectReference Include=\"%s\">\n", proj.Get() );
                    WriteF("      <Project>%s</Project>\n", guid.Get() );
                    Write( "    </ProjectReference>\n" );
                }
                else
                {
                    WriteF( "    <ProjectReference Include=\"%s\" />\n", proj.Get() );
                }
            }
        }
        {
            // References
            for ( const AString & reference : m_References )
            {
                WriteF( "    <Reference Include=\"%s\" />\n", reference.Get() );
            }
        }
        Write("  </ItemGroup>\n" );
    }

    // Globals
    Write( "  <PropertyGroup Label=\"Globals\">\n" );
    WritePGItem( "RootNamespace", m_RootNamespace );
    WritePGItem( "ProjectGuid", m_ProjectGuid );
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

    // Per-config Globals
    for ( const VSProjectConfig & config : configs )
    {
        const bool needSection = ( config.m_Keyword.IsEmpty() == false ) ||
                                 ( config.m_RootNamespace.IsEmpty() == false ) ||
                                 ( config.m_ApplicationType.IsEmpty() == false ) ||
                                 ( config.m_ApplicationTypeRevision.IsEmpty() == false ) ||
                                 ( config.m_TargetLinuxPlatform.IsEmpty() == false ) ||
                                 ( config.m_LinuxProjectType.IsEmpty() == false );
        if ( needSection )
        {
            WriteF( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"Globals\">\n", config.m_Config.Get(), config.m_Platform.Get() );
            WritePGItem( "Keyword", config.m_Keyword );
            WritePGItem( "RootNamespace", config.m_RootNamespace );
            WritePGItem( "ApplicationType", config.m_ApplicationType );
            WritePGItem( "ApplicationTypeRevision", config.m_ApplicationTypeRevision );
            WritePGItem( "TargetLinuxPlatform", config.m_TargetLinuxPlatform );
            WritePGItem( "LinuxProjectType", config.m_LinuxProjectType );
            Write( "  </PropertyGroup>\n" );
        }
    }

    // Default props
    Write( "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n" );

    // Configurations
    {
        for ( const VSProjectConfig & config : configs )
        {
            WriteF( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" Label=\"Configuration\">\n", config.m_Config.Get(), config.m_Platform.Get() );
            Write( "    <ConfigurationType>Makefile</ConfigurationType>\n" );
            Write( "    <UseDebugLibraries>false</UseDebugLibraries>\n" );

            // If a specific executable is specified, use that, otherwise try to auto-derive
            // the executable from the .Target
            AStackString<> localDebuggerCommand( config.m_LocalDebuggerCommand );
            if ( localDebuggerCommand.IsEmpty() )
            {
                // Get the executable path and make it project-relative
                const Node * debugTarget = ProjectGeneratorBase::FindExecutableDebugTarget( config.m_TargetNode );
                if ( debugTarget )
                {
                    ProjectGeneratorBase::GetRelativePath( projectBasePath, debugTarget->GetName(), localDebuggerCommand );
                }
            }

            WritePGItem( "PlatformToolset",                 config.m_PlatformToolset );
            WritePGItem( "LocalDebuggerCommandArguments",   config.m_LocalDebuggerCommandArguments );
            WritePGItem( "LocalDebuggerCommand",            localDebuggerCommand );
            WritePGItem( "LocalDebuggerEnvironment",        config.m_LocalDebuggerEnvironment );

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
        for ( const VSProjectConfig & config : configs )
        {
            WriteF("  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", config.m_Config.Get(), config.m_Platform.Get() );
            Write( "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n" );
            Write( "  </ImportGroup>\n" );
        }
    }

    // User macros
    Write( "  <PropertyGroup Label=\"UserMacros\" />\n" );

    // Property Group
    {
        for ( const VSProjectConfig & config : configs )
        {
            WriteF( "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", config.m_Config.Get(), config.m_Platform.Get() );

            if ( config.m_Keyword == "Linux" )
            {
                WritePGItem( "BuildCommandLine",                config.m_ProjectBuildCommand );
                WritePGItem( "ReBuildCommandLine",              config.m_ProjectRebuildCommand );
                WritePGItem( "CleanCommandLine",                config.m_ProjectCleanCommand );
            }
            else
            {
                WritePGItem( "NMakeBuildCommandLine",           config.m_ProjectBuildCommand );
                WritePGItem( "NMakeReBuildCommandLine",         config.m_ProjectRebuildCommand );
                WritePGItem( "NMakeCleanCommandLine",           config.m_ProjectCleanCommand );
            }
            WritePGItem( "AndroidApkLocation",              config.m_AndroidApkLocation );
            WritePGItem( "AndroidDebugComponent",           config.m_AndroidDebugComponent );
            WritePGItem( "AndroidDebugTarget",              config.m_AndroidDebugTarget );
            WritePGItem( "AndroidJdb",                      config.m_AndroidJdb );
            WritePGItem( "AndroidLldbPostAttachCommands",   config.m_AndroidLldbPostAttachCommands );
            WritePGItem( "AndroidLldbStartupCommands",      config.m_AndroidLldbStartupCommands );
            WritePGItem( "AndroidPostApkInstallCommands",   config.m_AndroidPostApkInstallCommands );
            WritePGItem( "AndroidPreApkInstallCommands",    config.m_AndroidPreApkInstallCommands );
            WritePGItem( "AndroidSymbolDirectories",        config.m_AndroidSymbolDirectories );
            WritePGItem( "AndroidWaitForDebugger",          config.m_AndroidWaitForDebugger );
            WritePGItem( "LaunchFlags",                     config.m_LaunchFlags );

            WritePGItem( "NMakeOutput",                     config.m_Output );
            const ObjectListNode * oln = nullptr;
            if ( config.m_PreprocessorDefinitions.IsEmpty() || config.m_IncludeSearchPath.IsEmpty() )
            {
                oln = ProjectGeneratorBase::FindTargetForIntellisenseInfo( config.m_TargetNode );
            }

            if ( config.m_PreprocessorDefinitions.IsEmpty() == false )
            {
                WritePGItem( "NMakePreprocessorDefinitions",    config.m_PreprocessorDefinitions );
            }
            else
            {
                if ( oln )
                {
                    Array< AString > defines;
                    ProjectGeneratorBase::ExtractDefines( oln->GetCompilerOptions(), defines, false );
                    AStackString<> definesStr;
                    ProjectGeneratorBase::ConcatIntellisenseOptions( defines, definesStr, nullptr, ";" );
                    WritePGItem( "NMakePreprocessorDefinitions", definesStr );
                }
            }
            StackArray< AString > includePaths;
            StackArray< AString > forceIncludes;
            if ( oln )
            {
                ProjectGeneratorBase::ExtractIncludePaths( oln->GetCompilerOptions(), includePaths, forceIncludes, false );
            }
            if ( config.m_IncludeSearchPath.IsEmpty() == false )
            {
                WritePGItem( "NMakeIncludeSearchPath",          config.m_IncludeSearchPath );
            }
            else if ( oln )
            {
                for ( AString & include : includePaths )
                {
                    ProjectGeneratorBase::GetRelativePath( projectBasePath, include, include );
                    #if !defined( __WINDOWS__ )
                        include.Replace( '/', '\\' ); // Convert to Windows-style slashes
                    #endif
                }
                AStackString<> includePathsStr;
                ProjectGeneratorBase::ConcatIntellisenseOptions( includePaths, includePathsStr, nullptr, ";" );
                WritePGItem( "NMakeIncludeSearchPath", includePathsStr );
            }
            if ( config.m_ForcedIncludes.IsEmpty() == false )
            {
                WritePGItem( "NMakeForcedIncludes",             config.m_ForcedIncludes );
            }
            else if ( oln )
            {
                for ( AString & forceInclude : forceIncludes )
                {
                    ProjectGeneratorBase::GetRelativePath( projectBasePath, forceInclude, forceInclude );
                    #if !defined( __WINDOWS__ )
                        forceInclude.Replace( '/', '\\' ); // Convert to Windows-style slashes
                    #endif
                }
                AStackString<> forceIncludePathsStr;
                ProjectGeneratorBase::ConcatIntellisenseOptions( forceIncludes, forceIncludePathsStr, nullptr, ";" );
                WritePGItem( "NMakeForcedIncludes", forceIncludePathsStr );
            }
            WritePGItem( "NMakeAssemblySearchPath",         config.m_AssemblySearchPath );
            WritePGItem( "NMakeForcedUsingAssemblies",      config.m_ForcedUsingAssemblies );
            if ( config.m_AdditionalOptions.IsEmpty() == false )
            {
                WritePGItem( "AdditionalOptions",               config.m_AdditionalOptions );
            }
            else
            {
                if ( oln )
                {
                    Array< AString > additionalOptions;
                    ProjectGeneratorBase::ExtractAdditionalOptions( oln->GetCompilerOptions(), additionalOptions );
                    AStackString<> additionalOptionsStr;
                    ProjectGeneratorBase::ConcatIntellisenseOptions( additionalOptions, additionalOptionsStr, nullptr, " " );
                    WritePGItem( "AdditionalOptions", additionalOptionsStr );
                }
            }
            WritePGItem( "Xbox360DebuggerCommand",          config.m_Xbox360DebuggerCommand );
            WritePGItem( "DebuggerFlavor",                  config.m_DebuggerFlavor );
            WritePGItem( "AumidOverride",                   config.m_AumidOverride );
            WritePGItem( "LocalDebuggerWorkingDirectory",   config.m_LocalDebuggerWorkingDirectory );
            WritePGItem( "IntDir",                          config.m_IntermediateDirectory );
            WritePGItem( "OutDir",                          config.m_OutputDirectory );
            WritePGItem( "PackagePath",                     config.m_PackagePath );
            WritePGItem( "AdditionalSymbolSearchPaths",     config.m_AdditionalSymbolSearchPaths );
            WritePGItem( "LayoutDir",                       config.m_LayoutDir );
            WritePGItem( "LayoutExtensionFilter",           config.m_LayoutExtensionFilter );
            WritePGItem( "RemoteDebuggerCommand",           config.m_RemoteDebuggerCommand );
            WritePGItem( "RemoteDebuggerCommandArguments",  config.m_RemoteDebuggerCommandArguments );
            WritePGItem( "RemoteDebuggerWorkingDirectory",  config.m_RemoteDebuggerWorkingDirectory );
            Write( "  </PropertyGroup>\n" );
        }
    }

    // ItemDefinition Groups
    {
        for ( const VSProjectConfig & config : configs )
        {
            WriteF("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", config.m_Config.Get(), config.m_Platform.Get() );
            Write( "    <BuildLog>\n" );
            if ( !config.m_BuildLogFile.IsEmpty() )
            {
                WritePGItem( "Path",          config.m_BuildLogFile );
            }
            else
            {
                Write( "      <Path />\n" );
            }
            Write( "    </BuildLog>\n" );
            if ( !config.m_DeploymentType.IsEmpty() || !config.m_DeploymentFiles.IsEmpty() )
            {
                Write( "    <Deploy>\n" );
                WritePGItem( "DeploymentType",          config.m_DeploymentType );
                WritePGItem( "DeploymentFiles",         config.m_DeploymentFiles );
                Write( "    </Deploy>\n" );
            }
            Write( "  </ItemDefinitionGroup>\n" );
        }
    }

    // footer
    Write("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n" );
    Write("  <ImportGroup Label=\"ExtensionTargets\">\n" );
    Write("  </ImportGroup>\n" );
    for ( const VSProjectImport & import : projectImports )
    {
        WriteF( "  <Import Condition=\"%s\" Project=\"%s\" />\n", import.m_Condition.Get(), import.m_Project.Get() );
    }
    Write( "</Project>" ); // carriage return at end

    m_OutputVCXProj = m_Tmp;
    return m_OutputVCXProj;
}

// GenerateVCXProjFilters
//------------------------------------------------------------------------------
const AString & VSProjectGenerator::GenerateVCXProjFilters( const AString & projectFile )
{
    m_Tmp.SetLength( 0 );

    // determine folder for project
    const char * lastProjSlash = projectFile.FindLast( NATIVE_SLASH );
    AStackString<> projectBasePath( projectFile.Get(), lastProjSlash ? lastProjSlash + 1 : projectFile.Get() );

    // Must already be canonicalized/de-duplicated
    ASSERT( m_FilePathsCanonicalized == true );

    // header
    Write( "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    Write( "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n" );

    // list of all folders
    Array< AString > folders( 1024 );
    Array< uint32_t > folderHashes( 1024 );

    // files
    {
        AStackString<> lastFolder;
        Write( "  <ItemGroup>\n" );
        for ( const VSProjectFilePair & filePathPair : m_Files )
        {
            // File reference (which VS uses to load from disk) is project-relative
            WriteF( "    <CustomBuild Include=\"%s\">\n", filePathPair.m_ProjectRelativePath.Get() );

            // get folder part, relative to base dir(s)
            const AString & fileName = filePathPair.m_AbsolutePath;
            AStackString<> folder;
            GetFolderPath( fileName, folder );

            if ( !folder.IsEmpty() )
            {
                WriteF( "      <Filter>%s</Filter>\n", folder.Get() );
            }
            Write( "    </CustomBuild>\n" );

            // add new folders
            if ( ( folder.IsEmpty() == false ) && ( folder != lastFolder ) )
            {
                lastFolder = folder;

                // Each unique path must be added, so FolderA/FolderB/FolderC
                // will result in 3 entries, FolderA, FolderA/FolderB and FolderA/FolderB/FolderC
                for (;;)
                {
                    // add this folder if not already added
                    const uint32_t folderHash = xxHash::Calc32( folder );
                    if ( folderHashes.Find( folderHash ) )
                    {
                        break; // If we've seen this folder, we've also seen the parent dirs
                    }

                    ASSERT( folder.IsEmpty() == false );
                    folders.Append( folder );
                    folderHashes.Append( folderHash );

                    // check parent folder
                    const char * lastSlash = folder.FindLast( BACK_SLASH );
                    if ( lastSlash == nullptr )
                    {
                        break; // no more parents
                    }
                    folder.SetLength( (uint32_t)( lastSlash - folder.Get() ) );
                }
            }
        }
        Write( "  </ItemGroup>\n" );
    }

    // folders
    {
        const size_t numFolders = folders.GetSize();
        for ( size_t i=0; i<numFolders; ++i )
        {
            const AString & folder = folders[ i ];
            const uint32_t folderHash = folderHashes[ i ];

            Write( "  <ItemGroup>\n" );
            WriteF( "    <Filter Include=\"%s\">\n", folder.Get() );
            WriteF( "      <UniqueIdentifier>{%08x-6c94-4f93-bc2a-7f5284b7d434}</UniqueIdentifier>\n", folderHash );
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
void VSProjectGenerator::Write( const char * string )
{
    const size_t len = AString::StrLen( string );

    // resize output buffer in large chunks to prevent re-sizing
    if ( m_Tmp.GetLength() + len > m_Tmp.GetReserved() )
    {
        m_Tmp.SetReserved( m_Tmp.GetReserved() + MEGABYTE );
    }

    m_Tmp.Append( string, len );
}

// Write
//------------------------------------------------------------------------------
void VSProjectGenerator::WriteF( const char * fmtString, ... )
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

    // Escape value
    AStackString<> escapedValue;
    for ( const char c : value )
    {
        switch ( c )
        {
            case '>':   escapedValue += "&gt;";     break;
            case '<':   escapedValue += "&lt;";     break;
            case '&':   escapedValue += "&amp;";    break;
            default:    escapedValue += c;          break;
        }
    }

    WriteF( "    <%s>%s</%s>\n", xmlTag, escapedValue.Get(), xmlTag );
}

// GetFolderPath
//------------------------------------------------------------------------------
void VSProjectGenerator::GetFolderPath( const AString & fileName, AString & folder ) const
{
    ASSERT( m_FilePathsCanonicalized );
    for ( const AString & basePath : m_BasePaths )
    {
        if ( fileName.BeginsWithI( basePath ) )
        {
            const char * begin = fileName.Get() + basePath.GetLength();
            const char * end = fileName.FindLast( BACK_SLASH );
            end = ( end ) ? end : fileName.GetEnd();
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

// CanonicalizeFilePaths
//------------------------------------------------------------------------------
void VSProjectGenerator::CanonicalizeFilePaths( const AString & projectBasePath )
{
    if ( m_FilePathsCanonicalized )
    {
        return;
    }

    // Base Paths are retained as absolute paths
    #if !defined( __WINDOWS__ )
        for ( AString & basePath : m_BasePaths )
        {
            basePath.Replace( FORWARD_SLASH, BACK_SLASH ); // Always Windows-style inside project
        }
    #endif

    // Files
    if ( m_Files.IsEmpty() == false )
    {
        // Canonicalize and make all paths relative to project
        Array< const VSProjectFilePair * > filePointers( m_Files.GetSize() );
        for ( VSProjectFilePair & filePathPair : m_Files )
        {
            ProjectGeneratorBase::GetRelativePath( projectBasePath, filePathPair.m_AbsolutePath, filePathPair.m_ProjectRelativePath );
            #if !defined( __WINDOWS__ )
                filePathPair.m_ProjectRelativePath.Replace( FORWARD_SLASH, BACK_SLASH ); // Always Windows-style inside project
            #endif
            filePointers.Append( &filePathPair );
        }

        // Sort filenames to allow finding de-duplication
        FileAscendingCompareIDeref sorter;
        filePointers.Sort( sorter );

        // Find unique files
        Array< VSProjectFilePair > uniqueFiles( m_Files.GetSize() );
        const VSProjectFilePair * prev = filePointers[ 0 ];
        uniqueFiles.Append( *filePointers[ 0 ] );
        const size_t numFiles = m_Files.GetSize();
        for ( size_t i=1; i<numFiles; ++i )
        {
            const VSProjectFilePair * current = filePointers[ i ];
            if ( current->m_ProjectRelativePath.EqualsI( prev->m_ProjectRelativePath ) )
            {
                continue;
            }
            uniqueFiles.Append( *current );

            prev = current;
        }

        // Keep uniquified list. Even if there were no duplicates
        // we keep the sorted list in order to have more consistent behaviour
        uniqueFiles.Swap( m_Files );
    }

    m_FilePathsCanonicalized = true;
}

//------------------------------------------------------------------------------
