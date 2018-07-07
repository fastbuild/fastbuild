// TestProjectGeneration.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// TestProjectGeneration
//------------------------------------------------------------------------------
class TestProjectGeneration : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void Test() const;
    void TestFunction() const;
    void TestFunction_NoRebuild() const;
    void TestFunction_Speed() const;

    // VCXProj
    void VCXProj_DefaultConfigs() const;
    void VCXProj_PerConfigOverrides() const;
    void VCXProj_HandleDuplicateFiles() const;
    void VCXProj_Folders() const;

    // XCode
    void XCode() const;

    // Intellisense/CodeSense
    void IntellisenseAndCodeSense() const;

    // Helpers
    void VCXProj_Intellisense_Check( const char * projectFile ) const;
    void XCodeProj_CodeSense_Check( const char * projectFile ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestProjectGeneration )
    REGISTER_TEST( Test )
    REGISTER_TEST( TestFunction )
    REGISTER_TEST( TestFunction_NoRebuild )
    REGISTER_TEST( TestFunction_Speed )
    REGISTER_TEST( VCXProj_DefaultConfigs )
    REGISTER_TEST( VCXProj_PerConfigOverrides )
    REGISTER_TEST( VCXProj_HandleDuplicateFiles )
    REGISTER_TEST( VCXProj_Folders )
    REGISTER_TEST( XCode )
    REGISTER_TEST( IntellisenseAndCodeSense )
REGISTER_TESTS_END

// Test
//------------------------------------------------------------------------------
void TestProjectGeneration::Test() const
{
    // work out where we are running, and find "Core"
    AStackString<> baseDir;
    TEST_ASSERT( FileIO::GetCurrentDir( baseDir ) );
    TEST_ASSERT( baseDir.FindI( "code" ) );
    #if defined( __WINDOWS__ )
        baseDir += "\\Core\\";
    #else
        baseDir += "/Core/";
    #endif
    Array< AString > baseDirs;
    baseDirs.Append( baseDir );

    VSProjectGenerator pg;

    // project name/guid
    AStackString<> name( "Core" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );
    pg.SetBasePaths( baseDirs );

    Array< VSProjectConfig > configs;
    VSProjectConfig cfg;

    // commands
    cfg.m_ProjectBuildCommand = "fbuild -cache $(Project)-$(Config)-$(Platform)";
    cfg.m_ProjectRebuildCommand = "fbuild -cache -clean $(Project)-$(Config)-$(Platform)";

    // debugger
    cfg.m_LocalDebuggerCommand = "$(SolutionDir)..\\..\\..\\tmp\\$(Platform)\\$(Config)\\Tools\\FBuild\\FBuildTest\\FBuildTest.exe";
    cfg.m_LocalDebuggerWorkingDirectory = "$(ProjectDir)";
    cfg.m_LocalDebuggerCommandArguments = "-verbose";
    cfg.m_LocalDebuggerEnvironment = "_NO_DEBUG_HEAP=1";

    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );
    cfg.m_Config = "Profile";
    configs.Append( cfg );
    cfg.m_Config = "Release";
    configs.Append( cfg );

    cfg.m_Platform = "x64";
    cfg.m_Config = "Debug";
    configs.Append( cfg );
    cfg.m_Config = "Profile";
    configs.Append( cfg );
    cfg.m_Config = "Release";
    configs.Append( cfg );

    // files
    Array< AString > files;
    FileIO::GetFiles( baseDir, AStackString<>( "*.cpp" ), true, &files );
    FileIO::GetFiles( baseDir, AStackString<>( "*.h" ), true, &files );
    pg.AddFiles( files );

    // fileTypes
    Array< VSProjectFileType > fileTypes;
    {
        VSProjectFileType ft;
        ft.m_FileType = "CppForm";
        ft.m_Pattern = "*\\Core\\Strings\\AString.h";
        fileTypes.Append( ft );
    }

    FBuild fBuild; // needed for NodeGraph::CleanPath

    AStackString<> projectFile( "../../../../tmp/Test/ProjectGeneration/Core.vcxproj" );
    AStackString<> projectFileClean;
    NodeGraph::CleanPath( projectFile, projectFileClean );

    const AString & vcxproj = pg.GenerateVCXProj( projectFileClean, configs, fileTypes );
    const AString & filters = pg.GenerateVCXProjFilters( projectFileClean );

    TEST_ASSERT( FileIO::EnsurePathExists( AStackString<>( "../../../../tmp/Test/ProjectGeneration/" ) ) );

    FileStream f;
    TEST_ASSERT( f.Open( projectFileClean.Get(), FileStream::WRITE_ONLY ) );
    TEST_ASSERT( f.Write( vcxproj.Get(), vcxproj.GetLength() ) == vcxproj.GetLength() );
    f.Close();
    TEST_ASSERT( f.Open( "../../../../tmp/Test/ProjectGeneration/Core.vcxproj.filters", FileStream::WRITE_ONLY ) );
    TEST_ASSERT( f.Write( filters.Get(), filters.GetLength() ) == filters.GetLength() );
}

// TestFunction
//------------------------------------------------------------------------------
void TestProjectGeneration::TestFunction() const
{
    AStackString<> project( "../tmp/Test/ProjectGeneration/testproj.vcxproj" );
    AStackString<> solution( "../tmp/Test/ProjectGeneration/testsln.sln" );
    AStackString<> filters( "../tmp/Test/ProjectGeneration/testproj.vcxproj.filters" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( project );
    EnsureFileDoesNotExist( solution );
    EnsureFileDoesNotExist( filters );

    // do build
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestSln" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/ProjectGeneration/fbuild.fdb" ) );

    EnsureFileExists( project );
    EnsureFileExists( solution );
    EnsureFileExists( filters );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::VCXPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::SLN_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 4,     4 );
}

// TestFunction_NoRebuild
//------------------------------------------------------------------------------
void TestProjectGeneration::TestFunction_NoRebuild() const
{
    AStackString<> project( "../tmp/Test/ProjectGeneration/testproj.vcxproj" );
    AStackString<> filters( "../tmp/Test/ProjectGeneration/testproj.vcxproj.filters" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Data/TestProjectGeneration/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/ProjectGeneration/fbuild.fdb" ) );

    // Delete old files from previous runs
    EnsureFileExists( project );
    EnsureFileExists( filters );

    // Projects and Solutions must be "built" every time, but only write files when they change
    // so record the time before and after
    uint64_t dateTime1 = FileIO::GetFileLastWriteTime( project );
    uint64_t dateTime2 = FileIO::GetFileLastWriteTime( filters );

    // NTFS file resolution is 100ns and HFS is 1 second,
    // so sleep long enough to ensure an invalid write would modify the time
    #if defined( __WINDOWS__ )
        Thread::Sleep( 1 ); // 1ms
    #elif defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #elif defined( __LINUX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of ext2/ext3/reiserfs and time caching used by used by others
    #endif

    // do build
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestProj" ) ) );

    // Make sure files have not been changed
    TEST_ASSERT( dateTime1 == FileIO::GetFileLastWriteTime( project ) );
    TEST_ASSERT( dateTime2 == FileIO::GetFileLastWriteTime( filters ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::VCXPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     3 );
}

// TestFunction_Speed
//------------------------------------------------------------------------------
void TestProjectGeneration::TestFunction_Speed() const
{
    VSProjectGenerator pg;
    AStackString<> baseDir;
    GetCodeDir( baseDir );
    Array< AString > baseDirs;
    baseDirs.Append( baseDir );

    // project name
    AStackString<> name( "Big" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );
    pg.SetBasePaths( baseDirs );

    // platforms
    Array< VSProjectConfig > configs;
    configs.SetCapacity( 6 );
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";   cfg.m_Config = "Debug";     configs.Append( cfg );
    cfg.m_Platform = "Win32";   cfg.m_Config = "Profile";   configs.Append( cfg );
    cfg.m_Platform = "Win32";   cfg.m_Config = "Release";   configs.Append( cfg );
    cfg.m_Platform = "x64";     cfg.m_Config = "Profile";   configs.Append( cfg );
    cfg.m_Platform = "x64";     cfg.m_Config = "Release";   configs.Append( cfg );

    // files
    Array< AString > files;
    FileIO::GetFiles( baseDir, AStackString<>( "*" ), true, &files );
    pg.AddFiles( files );

    Array< VSProjectFileType > fileTypes;
    {
        VSProjectFileType ft;
        ft.m_FileType = "CppForm";
        ft.m_Pattern = "Code\\Forms\\*.h";
        fileTypes.Append( ft );
        ft.m_FileType = "CppControl";
        ft.m_Pattern = "Controls\\*.h";
        fileTypes.Append( ft );
    }

    AStackString<> projectFileName( "dummy.vcxproj" );

    {
        Timer t;
        for ( size_t i = 0; i < 5; ++i )
        {
            pg.GenerateVCXProj( projectFileName, configs, fileTypes );
        }
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj        : %2.3fs\n", time );
    }
    {
        Timer t;
        for ( size_t i = 0; i < 5; ++i )
        {
            pg.GenerateVCXProjFilters( projectFileName );
        }
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj.filters: %2.3fs\n", time );
    }
}

// IntellisenseAndCodeSense
//------------------------------------------------------------------------------
void TestProjectGeneration::IntellisenseAndCodeSense() const
{
    // Parse bff
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Intellisense/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Generate project
    TEST_ASSERT( fBuild.Build( AStackString<>( "Intellisense" ) ) );

    // Ensure VS Intellisense info is present
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/ObjectList.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Library.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Executable.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Test.vcxproj" );

    // Ensure XCode CodeSense info is present
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/ObjectList.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Library.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Executable.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Test.xcodeproj/project.pbxproj" );
}

// VCXProj_Intellisense_Check
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_Intellisense_Check( const char * projectFile ) const
{
    // Read Project
    FileStream f;
    TEST_ASSERT( f.Open( projectFile, FileStream::READ_ONLY ) );
    AString buffer;
    buffer.SetLength( (uint32_t)f.GetFileSize() );
    TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
    Array< AString > tokens;
    buffer.Tokenize( tokens, '\n' );

    // Check
    bool definesOk = false;
    bool includesOk = false;
    for ( const AString & token : tokens )
    {
        if ( token.Find( "NMakePreprocessorDefinitions" ) )
        {
            TEST_ASSERT( token.Find( "INTELLISENSE_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_SPACE_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_SLASH_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_SLASH_SPACE_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_QUOTED_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_QUOTED_SPACE_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_QUOTED_SLASH_DEFINE" ) );
            TEST_ASSERT( token.Find( "INTELLISENSE_QUOTED_SLASH_SPACE_DEFINE" ) );
            definesOk = true;
        }
        else if ( token.Find( "NMakeIncludeSearchPath" ) )
        {
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Space\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Slash\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Slash\\Space\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Quoted\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Quoted\\Space\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Quoted\\Slash\\Path" ) );
            TEST_ASSERT( token.Find( "Intellisense\\Include\\Quoted\\Slash\\Space\\Path" ) );
            includesOk = true;
        }
    }
    TEST_ASSERT( definesOk );
    TEST_ASSERT( includesOk );
}


// XCodeProj_CodeSense_Check
//------------------------------------------------------------------------------
void TestProjectGeneration::XCodeProj_CodeSense_Check( const char * projectFile ) const
{
    // Read Project
    FileStream f;
    TEST_ASSERT( f.Open( projectFile, FileStream::READ_ONLY ) );
    AString buffer;
    buffer.SetLength( (uint32_t)f.GetFileSize() );
    TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
    Array< AString > tokens;
    buffer.Tokenize( tokens, '\n' );

    // Check
    const size_t NUM_DEFINES = 8;
    bool definesOk[ NUM_DEFINES ] = { false, false, false, false, false, false, false, false };
    const size_t NUM_INCLUDES = 8;
    bool includesOk[ NUM_INCLUDES ] = { false, false, false, false, false, false, false, false };
    bool inDefineSection = false;
    bool inIncludeSection = false;
    for ( const AString & token : tokens )
    {
        // Check for start/end of sections
        if ( token.Find( "GCC_PREPROCESSOR_DEFINITIONS" ) )
        {
            inDefineSection = true;
            continue;
        }
        if ( token.Find( "USER_HEADER_SEARCH_PATHS" ) )
        {
            inIncludeSection = true;
            continue;
        }
        if ( token == "\t\t\t\t);" )
        {
            if ( inDefineSection )
            {
                inDefineSection = false;
            }
            else if ( inIncludeSection )
            {
                inIncludeSection = false;
            }
            continue;
        }

        // Defines
        if ( inDefineSection )
        {
            if ( token.Find( "INTELLISENSE_DEFINE" ) )                      { definesOk[ 0 ] = true; }
            if ( token.Find( "INTELLISENSE_SPACE_DEFINE" ) )                { definesOk[ 1 ] = true; }
            if ( token.Find( "INTELLISENSE_SLASH_DEFINE" ) )                { definesOk[ 2 ] = true; }
            if ( token.Find( "INTELLISENSE_SLASH_SPACE_DEFINE" ) )          { definesOk[ 3 ] = true; }
            if ( token.Find( "INTELLISENSE_QUOTED_DEFINE" ) )               { definesOk[ 4 ] = true; }
            if ( token.Find( "INTELLISENSE_QUOTED_SPACE_DEFINE" ) )         { definesOk[ 5 ] = true; }
            if ( token.Find( "INTELLISENSE_QUOTED_SLASH_DEFINE" ) )         { definesOk[ 6 ] = true; }
            if ( token.Find( "INTELLISENSE_QUOTED_SLASH_SPACE_DEFINE" ) )   { definesOk[ 7 ] = true; }
            continue;
        }

        // Includes
        if ( inIncludeSection )
        {
            if ( token.Find( "Intellisense/Include/Path" ) )                    { includesOk[ 0 ] = true; }
            if ( token.Find( "Intellisense/Include/Space/Path" ) )              { includesOk[ 1 ] = true; }
            if ( token.Find( "Intellisense/Include/Slash/Path" ) )              { includesOk[ 2 ] = true; }
            if ( token.Find( "Intellisense/Include/Slash/Space/Path" ) )        { includesOk[ 3 ] = true; }
            if ( token.Find( "Intellisense/Include/Quoted/Path" ) )             { includesOk[ 4 ] = true; }
            if ( token.Find( "Intellisense/Include/Quoted/Space/Path" ) )       { includesOk[ 5 ] = true; }
            if ( token.Find( "Intellisense/Include/Quoted/Slash/Path" ) )       { includesOk[ 6 ] = true; }
            if ( token.Find( "Intellisense/Include/Quoted/Slash/Space/Path" ) ) { includesOk[ 7 ] = true; }
            continue;
        }
    }

    // Check we found them all
    for ( size_t i=0; i<NUM_DEFINES; ++i )
    {
        TEST_ASSERT( definesOk[ i ]  );
    }
    for ( size_t i=0; i<NUM_INCLUDES; ++i )
    {
        TEST_ASSERT( includesOk[ i ]  );
    }
}

// VCXProj_DefaultConfigs
//
//  - If no projects are specified, then some default configs should be created.
//  - And properties set at the project level should be present in each default
//    config
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_DefaultConfigs() const
{
    AStackString<> project( "../tmp/Test/ProjectGeneration/VCXProj_DefaultConfigs/DefaultConfigs.vcxproj" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/VCXProj_DefaultConfigs/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete files from previous builds
    EnsureFileDoesNotExist( project );

    // do build
    TEST_ASSERT( fBuild.Build( AStackString<>( "DefaultConfigs" ) ) );

    // Load the generate project into memory
    FileStream f;
    TEST_ASSERT( f.Open( project.Get(), FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( fileSize );
    TEST_ASSERT( f.ReadBuffer( fileContents.Get(), fileSize ) == fileSize );

    // Ensure project has 4 configs (Debug/Release x Win32/x64)
    TEST_ASSERT( fileContents.Find( "<ProjectConfiguration Include=\"Debug|Win32\">" ) );
    TEST_ASSERT( fileContents.Find( "<ProjectConfiguration Include=\"Debug|x64\">" ) );
    TEST_ASSERT( fileContents.Find( "<ProjectConfiguration Include=\"Release|Win32\">" ) );
    TEST_ASSERT( fileContents.Find( "<ProjectConfiguration Include=\"Release|x64\">" ) );
    const uint32_t numConfigs = fileContents.Replace( "<ProjectConfiguration Include", "" );
    TEST_ASSERT( numConfigs == 4 );

    // Ensure each config inherited the project-level settings
    const uint32_t buildCmdCount    = fileContents.Replace( "<NMakeBuildCommandLine>BASE_BUILD_COMMAND</NMakeBuildCommandLine>", "" );
    const uint32_t rebuildCmdCount  = fileContents.Replace( "<NMakeReBuildCommandLine>BASE_REBUILD_COMMAND</NMakeReBuildCommandLine>", "" );
    TEST_ASSERT( buildCmdCount == numConfigs );
    TEST_ASSERT( rebuildCmdCount == numConfigs );
}

// VCXProj_PerConfigOverrides
//
//  - Settings not set at the config level should be inherited from the project
//  - Settings set at the config level should override those set on the project
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_PerConfigOverrides() const
{
    AStackString<> project( "../tmp/Test/ProjectGeneration/VCXProj_PerConfigOverrides/PerConfigOverrides.vcxproj" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/VCXProj_PerConfigOverrides/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete files from previous builds
    EnsureFileDoesNotExist( project );

    // do build
    TEST_ASSERT( fBuild.Build( AStackString<>( "PerConfigOverrides" ) ) );

    // Load the generate project into memory
    FileStream f;
    TEST_ASSERT( f.Open( project.Get(), FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( fileSize );
    TEST_ASSERT( f.ReadBuffer( fileContents.Get(), fileSize ) == fileSize );

    // Ensure project has 1 config
    TEST_ASSERT( fileContents.Find( "<ProjectConfiguration Include=\"Debug|Win32\">" ) );
    const uint32_t numConfigs = fileContents.Replace( "<ProjectConfiguration Include", "" );
    TEST_ASSERT( numConfigs == 1 );

    // Check options were overriden in the config
    const uint32_t buildCmdCount    = fileContents.Replace( "<NMakeBuildCommandLine>OVERRIDDEN_BUILD_COMMAND</NMakeBuildCommandLine>", "" );
    TEST_ASSERT( buildCmdCount == numConfigs );

    // Check non-overridden options were inherited
    const uint32_t rebuildCmdCount  = fileContents.Replace( "<NMakeReBuildCommandLine>BASE_REBUILD_COMMAND</NMakeReBuildCommandLine>", "" );
    TEST_ASSERT( rebuildCmdCount == numConfigs );

    // Check option set only on config
    const uint32_t cleanCmdCount  = fileContents.Replace( "<NMakeCleanCommandLine>CONFIG_CLEAN_COMMAND</NMakeCleanCommandLine>", "" );
    TEST_ASSERT( cleanCmdCount == numConfigs );
}

// VCXProj_HandleDuplicateFiles
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_HandleDuplicateFiles() const
{
    FBuild fb; // For CleanPath

    VSProjectGenerator pg;

    // Project name
    AStackString<> name( "Project" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );

    // Base dir

    // Platforms
    Array< VSProjectConfig > configs;
    configs.SetCapacity( 6 );
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // Files 
    pg.AddFile( AStackString<>( "File.cpp" ) );
    pg.AddFile( AStackString<>( "file.cpp" ) );                 // Duplicate with case difference
    pg.AddFile( AStackString<>( "File.cpp" ) );                 // Exact duplicate
    pg.AddFile( AStackString<>( "../Code/File.cpp" ) );         // Duplicate with path difference
    pg.AddFile( AStackString<>( "../Dir/../Code/File.cpp" ) );  // Duplicate with path difference
    
    AStackString<> projectFileName( "dummy.vcxproj" );

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >() ) );
        TEST_ASSERT( proj.Replace( "File.cpp", "" ) == 1 );
        TEST_ASSERT( proj.FindI( "File.cpp" ) == nullptr );
    }

    // Check vcxproj.filters
    {
        AStackString<> filter( pg.GenerateVCXProjFilters( projectFileName ) );
        TEST_ASSERT( filter.Replace( "File.cpp", "" ) == 1 );
        TEST_ASSERT( filter.FindI( "File.cpp" ) == nullptr );
    }
}

// VCXProj_Folders
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_Folders() const
{
    FBuild fb; // For CleanPath

    VSProjectGenerator pg;

    // Project name
    AStackString<> name( "Project" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );

    // Base dir
    AStackString<> baseDir;
    GetCodeDir( baseDir );
    Array< AString > baseDirs;
    baseDirs.Append( baseDir );
    pg.SetBasePaths( baseDirs );

    // Platforms
    Array< VSProjectConfig > configs;
    configs.SetCapacity( 6 );
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // Files in various sub-dirs
    pg.AddFile( AStackString<>( "FolderA/AFile.cpp" ) );
    pg.AddFile( AStackString<>( "FolderA/BFolder/SubDir/AFile.cpp" ) );
    pg.AddFile( AStackString<>( "FolderA/ZFile.cpp" ) );
    pg.AddFile( AStackString<>( "FolderA/ZFolder/SubDir/AFile.cpp" ) );
    pg.AddFile( AStackString<>( "FolderZ/ZFile.cpp" ) );

    // Dirs which are substrings of each other but unique
    pg.AddFile( AStackString<>( "Data/TestPrecompiledHeaders/CacheUniqueness2/PrecompiledHeader.cpp" ) );
    pg.AddFile( AStackString<>( "Data/TestPrecompiledHeaders/CacheUniqueness/PrecompiledHeader.cpp" ) );
    
    AStackString<> projectFileName;
    projectFileName.Format( "%sdummy.vcxproj", baseDir.Get() );

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >() ) );
        TEST_ASSERT( proj.Replace( "AFile.cpp", "" ) == 3 );
        TEST_ASSERT( proj.FindI( "AFile.cpp" ) == nullptr );
        TEST_ASSERT( proj.Replace( "ZFile.cpp", "" ) == 2 );
        TEST_ASSERT( proj.FindI( "ZFile.cpp" ) == nullptr );
        TEST_ASSERT( proj.Replace( "PrecompiledHeader.cpp", "" ) == 2 );
        TEST_ASSERT( proj.FindI( "PrecompiledHeader.cpp" ) == nullptr );
    }

    // Check vcxproj.filters
    {
        AStackString<> filter( pg.GenerateVCXProjFilters( projectFileName ) );

        // Should have a folder entry for each unique folder, including
        // directories who don't directly have any files in them
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderA\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderA\\BFolder\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderA\\BFolder\\SubDir\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderA\\ZFolder\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderA\\ZFolder\\SubDir\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"FolderZ\">", "" ) == 1 );

        // Ensure dirs which are substrings of each other but unique
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Data\\TestPrecompiledHeaders\\CacheUniqueness2\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Data\\TestPrecompiledHeaders\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Data\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Data\\TestPrecompiledHeaders\\CacheUniqueness\">", "" ) == 1 );

        // Ensure test has accounted for all paths
        TEST_ASSERT( filter.Find( "Filter Include=" ) == nullptr );
    }
}

// XCode
//------------------------------------------------------------------------------
void TestProjectGeneration::XCode() const
{
    AStackString<> project( "../tmp/Test/ProjectGeneration/Test.xcodeproj/project.pbxproj" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/xcodeproject.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete files from previous builds
    EnsureFileDoesNotExist( project );

    // do build
    TEST_ASSERT( fBuild.Build( AStackString<>( "XCodeProj" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 0,     0,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::XCODEPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     2 );
}

//------------------------------------------------------------------------------
