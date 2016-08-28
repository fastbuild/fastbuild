// TestProjectGeneration.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

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

    // XCode
    void XCode() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestProjectGeneration )
    REGISTER_TEST( Test )
    REGISTER_TEST( TestFunction )
    REGISTER_TEST( TestFunction_NoRebuild )
    REGISTER_TEST( TestFunction_Speed )
    REGISTER_TEST( XCode )
REGISTER_TESTS_END

// Test
//------------------------------------------------------------------------------
void TestProjectGeneration::Test() const
{
    // work out where we are running, and find "Core"
    AStackString<> oldDir;
    TEST_ASSERT( FileIO::GetCurrentDir( oldDir ) );
    AStackString<> baseDir( oldDir );
    #if defined( __WINDOWS__ )
        const char * codeLoc = baseDir.FindI( "\\code\\" );
    #else
        const char * codeLoc = baseDir.FindI( "/code/" );
    #endif
    TEST_ASSERT( codeLoc );
    baseDir.SetLength( (uint32_t)( codeLoc - baseDir.Get() ) );
    #if defined( __WINDOWS__ )
        baseDir += "\\Code\\Core\\";
    #else
        baseDir += "/Code/Core/";
    #endif
    Array< AString > baseDirs;
    baseDirs.Append( baseDir );

    VSProjectGenerator pg;

    // project name
    pg.SetProjectName( AStackString<>( "Core" ) );
    pg.SetBasePaths( baseDirs );

    Array< VSProjectConfig > configs;
    VSProjectConfig cfg;

    // commands
    cfg.m_BuildCommand = "fbuild -cache $(Project)-$(Config)-$(Platform)";
    cfg.m_RebuildCommand = "fbuild -cache -clean $(Project)-$(Config)-$(Platform)";

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
    AStackString<> project( "../../../../tmp/Test/ProjectGeneration/testproj.vcxproj" );
    AStackString<> solution( "../../../../tmp/Test/ProjectGeneration/testsln.sln" );
    AStackString<> filters( "../../../../tmp/Test/ProjectGeneration/testproj.vcxproj.filters" );
    EnsureFileDoesNotExist( project );
    EnsureFileDoesNotExist( solution );
    EnsureFileDoesNotExist( filters );

    FBuildOptions options;
    options.m_ConfigFile = "Data/TestProjectGeneration/fbuild.bff";
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "TestSln" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/ProjectGeneration/fbuild.fdb" ) );

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
    AStackString<> project( "../../../../tmp/Test/ProjectGeneration/testproj.vcxproj" );
    AStackString<> filters( "../../../../tmp/Test/ProjectGeneration/testproj.vcxproj.filters" );
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
    #else
        Thread::Sleep( 1000 ); // 1 second
    #endif

    // do build
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestProjectGeneration/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/ProjectGeneration/fbuild.fdb" ) );

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
    AStackString<> baseDir( "C:\\Windows\\System32" );
    Array< AString > baseDirs;
    baseDirs.Append( baseDir );

    // project name
    pg.SetProjectName( AStackString<>( "Big" ) );
    pg.SetBasePaths( baseDirs );

    // platforms
    Array< VSProjectConfig > configs;
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // files (about 5,000)
    Array< AString > files;
    FileIO::GetFiles( baseDir, AStackString<>( "*.mui" ), true, &files );
    FileIO::GetFiles( baseDir, AStackString<>( "*.exe" ), true, &files );
    FileIO::GetFiles( baseDir, AStackString<>( "*.dll" ), true, &files );
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

    AStackString<> projectFileName( "C:\\Windows\\System\\dummy.vcxproj" );

    {
        Timer t;
        pg.GenerateVCXProj( projectFileName, configs, fileTypes );
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj        : %2.3fs\n", time );
    }
    {
        Timer t;
        pg.GenerateVCXProjFilters( projectFileName );
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj.filters: %2.3fs\n", time );
    }
}

// XCode
//------------------------------------------------------------------------------
void TestProjectGeneration::XCode() const
{
    AStackString<> project( "../../../../tmp/Test/ProjectGeneration/Test.xcodeproj/project.pbxproj" );
    EnsureFileDoesNotExist( project );

    // do build
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestProjectGeneration/xcodeproject.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "XCodeProj" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 0,     0,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::XCODEPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     2 );
}

//------------------------------------------------------------------------------
