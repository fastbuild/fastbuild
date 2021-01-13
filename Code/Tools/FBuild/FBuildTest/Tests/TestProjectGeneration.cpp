// TestProjectGeneration.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Random.h"
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
    void FindExecutableTarget() const;

    // VCXProj
    void VCXProj_DefaultConfigs() const;
    void VCXProj_PerConfigOverrides() const;
    void VCXProj_HandleDuplicateFiles() const;
    void VCXProj_Folders() const;
    void VCXProj_ProjectRelativePaths() const;
    void VCXProj_ProjectRelativePaths2() const;

    // Solution
    void Solution_Empty() const;
    void Solution_SolutionRelativePaths() const;
    void Solution_BuildAndDeploy_None() const;
    void Solution_BuildAndDeploy_Project() const;
    void Solution_BuildAndDeploy_PerSolutionConfig() const;
    void Solution_Items() const;

    // VSExternalProj
    void VSExternalProj_ExternalProject() const;
    void VSExternalProj_ExternalProjectWrongData() const;
    void VSExternalProj_ExternalProject_MissingProjectGUID() const;

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
    REGISTER_TEST( FindExecutableTarget )
    REGISTER_TEST( VCXProj_DefaultConfigs )
    REGISTER_TEST( VCXProj_PerConfigOverrides )
    REGISTER_TEST( VCXProj_HandleDuplicateFiles )
    REGISTER_TEST( VCXProj_Folders )
    REGISTER_TEST( VCXProj_ProjectRelativePaths )
    REGISTER_TEST( VCXProj_ProjectRelativePaths2 )
    REGISTER_TEST( Solution_Empty )
    REGISTER_TEST( Solution_SolutionRelativePaths )
    REGISTER_TEST( Solution_BuildAndDeploy_None )
    REGISTER_TEST( Solution_BuildAndDeploy_Project )
    REGISTER_TEST( Solution_BuildAndDeploy_PerSolutionConfig )
    REGISTER_TEST( Solution_Items )
    REGISTER_TEST( VSExternalProj_ExternalProject )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( VSExternalProj_ExternalProjectWrongData )
    #endif
    REGISTER_TEST( VSExternalProj_ExternalProject_MissingProjectGUID )
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

    // Project Imports
    Array< VSProjectImport > projectImports;

    FBuild fBuild; // needed for NodeGraph::CleanPath

    AStackString<> projectFile( "../../../../tmp/Test/ProjectGeneration/Core.vcxproj" );
    AStackString<> projectFileClean;
    NodeGraph::CleanPath( projectFile, projectFileClean );

    const AString & vcxproj = pg.GenerateVCXProj( projectFileClean, configs, fileTypes, projectImports );
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
    TEST_ASSERT( fBuild.Build( "TestSln" ) );
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
    TEST_ASSERT( fBuild.Build( "TestProj" ) );

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

    // Generate a large list of files
    Random r( 1234567 ); // Deterministic seed
    const size_t numFiles = 5000;
    const size_t maxSubDirDepth = 8;
    Array< AString > files( numFiles, false );
    for ( size_t i = 0; i < numFiles; ++i )
    {
        AStackString<> fileName( baseDir );

        // Add to different sub-directories
        const size_t subDirDepth = r.GetRandIndex( maxSubDirDepth );
        for ( size_t j = 0; j < subDirDepth; ++j )
        {
            fileName.AppendFormat( "%02x/", r.GetRand() & 0xFF );
        }

        // Unique file name
        fileName.AppendFormat( "%08x.cpp", (uint32_t)i );

        files.Append( fileName );
    }
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

    Array< VSProjectImport > projectImports;

    AStackString<> projectFileName;
    projectFileName.Format( "%s//dummy.vcxproj", baseDir.Get() );
    PathUtils::FixupFilePath( projectFileName );

    {
        Timer t;
        for ( size_t i = 0; i < 5; ++i )
        {
            pg.GenerateVCXProj( projectFileName, configs, fileTypes, projectImports );
        }
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj        : %2.3fs\n", (double)time );
    }
    {
        Timer t;
        for ( size_t i = 0; i < 5; ++i )
        {
            pg.GenerateVCXProjFilters( projectFileName );
        }
        float time = t.GetElapsed();
        OUTPUT( "Gen vcxproj.filters: %2.3fs\n", (double)time );
    }
}

// FindExecutableTarget
//
// When generating projects we sometimes need the path executable related to
// the project in a given config (to set the executable to debug for example)
//------------------------------------------------------------------------------
void TestProjectGeneration::FindExecutableTarget() const
{
    // Parse bff
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/FindExecutableTarget/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Alias -> Test -> Executable
    //  - This should return the Executable
    {
        const Node * testAlias = fBuild.GetNode( "AliasToTest" );
        TEST_ASSERT( testAlias );
        const Node * exe = ProjectGeneratorBase::FindExecutableDebugTarget( testAlias );
        TEST_ASSERT( exe );
        TEST_ASSERT( exe->GetType() == Node::EXE_NODE );
    }

    // Alias -> Copy -> Executable
    //  - This should return the Copy of the Executable (if making a copy
    //    it's likely a "staging dir" is being created and you want to
    //    collect dependencies in that folder and run from that folder)
    {
        const Node * copyAlias = fBuild.GetNode( "AliasToCopy" );
        TEST_ASSERT( copyAlias );
        const Node * copy = ProjectGeneratorBase::FindExecutableDebugTarget( copyAlias );
        TEST_ASSERT( copy );
        TEST_ASSERT( copy->GetType() == Node::COPY_FILE_NODE );
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
    TEST_ASSERT( fBuild.Build( "Intellisense" ) );

    // Ensure VS Intellisense info is present
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/ObjectList.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Library.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Executable.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Test.vcxproj" );
    VCXProj_Intellisense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Copy.vcxproj" );

    // Ensure XCode CodeSense info is present
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/ObjectList.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Library.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Executable.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Test.xcodeproj/project.pbxproj" );
    XCodeProj_CodeSense_Check( "../tmp/Test/ProjectGeneration/Intellisense/Copy.xcodeproj/project.pbxproj" );
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
    bool additionalOptionsOk = false;
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
        else if ( token.Find( "<NMakeIncludeSearchPath>" ) )
        {
            const char * pos = token.Find( "<NMakeIncludeSearchPath>" );

            const char * openTagEnd = token.Find( '>', pos );
            TEST_ASSERT( openTagEnd != nullptr );

            const char * closeTagBegin = token.FindLast( "</NMakeIncludeSearchPath>" );
            TEST_ASSERT( closeTagBegin != nullptr );
            TEST_ASSERT( openTagEnd < closeTagBegin );

            const AStackString<> tagValue( openTagEnd + 1, closeTagBegin );
            StackArray< AString > paths;
            tagValue.Tokenize( paths, ';' );

            // We only interested in checking relative ordering of paths that contain "Intellisense\\".
            Array< AString > includes;
            for ( const AString & path : paths )
            {
                const char * pathStartPos = path.Find( "Intellisense\\" );
                if ( pathStartPos != nullptr )
                {
                    // Check that we separated path from the option name correctly.
                    TEST_ASSERT( ( pathStartPos == path.Get() ) || ( pathStartPos[ -1 ] == '\\' ) );

                    includes.EmplaceBack( pathStartPos, path.GetEnd() );
                }
            }

            TEST_ASSERT( includes.GetSize() == 40 );
            TEST_ASSERT( includes[  0 ] == "Intellisense\\Include\\Path" );
            TEST_ASSERT( includes[  1 ] == "Intellisense\\Include\\Space\\Path" );
            TEST_ASSERT( includes[  2 ] == "Intellisense\\Include\\Slash\\Path" );
            TEST_ASSERT( includes[  3 ] == "Intellisense\\Include\\Slash\\Space\\Path" );
            TEST_ASSERT( includes[  4 ] == "Intellisense\\Include\\Quoted\\Path" );
            TEST_ASSERT( includes[  5 ] == "Intellisense\\Include\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[  6 ] == "Intellisense\\Include\\Quoted\\Slash\\Path" );
            TEST_ASSERT( includes[  7 ] == "Intellisense\\Include\\Quoted\\Slash\\Space\\Path" );
            TEST_ASSERT( includes[  8 ] == "Intellisense\\SystemInclude\\Path" );
            TEST_ASSERT( includes[  9 ] == "Intellisense\\SystemInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 10 ] == "Intellisense\\SystemInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 11 ] == "Intellisense\\SystemInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 12 ] == "Intellisense\\SystemAfterInclude\\Path" );
            TEST_ASSERT( includes[ 13 ] == "Intellisense\\SystemAfterInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 14 ] == "Intellisense\\SystemAfterInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 15 ] == "Intellisense\\SystemAfterInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 16 ] == "Intellisense\\MSVCInclude\\Path" );
            TEST_ASSERT( includes[ 17 ] == "Intellisense\\MSVCInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 18 ] == "Intellisense\\MSVCInclude\\Slash\\Path" );
            TEST_ASSERT( includes[ 19 ] == "Intellisense\\MSVCInclude\\Slash\\Space\\Path" );
            TEST_ASSERT( includes[ 20 ] == "Intellisense\\MSVCInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 21 ] == "Intellisense\\MSVCInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 22 ] == "Intellisense\\MSVCInclude\\Quoted\\Slash\\Path" );
            TEST_ASSERT( includes[ 23 ] == "Intellisense\\MSVCInclude\\Quoted\\Slash\\Space\\Path" );
            TEST_ASSERT( includes[ 24 ] == "Intellisense\\DirAfterInclude\\Path" );
            TEST_ASSERT( includes[ 25 ] == "Intellisense\\DirAfterInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 26 ] == "Intellisense\\DirAfterInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 27 ] == "Intellisense\\DirAfterInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 28 ] == "Intellisense\\QuoteInclude\\Path" );
            TEST_ASSERT( includes[ 29 ] == "Intellisense\\QuoteInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 30 ] == "Intellisense\\QuoteInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 31 ] == "Intellisense\\QuoteInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 32 ] == "Intellisense\\MSVCExternalInclude\\Path" );
            TEST_ASSERT( includes[ 33 ] == "Intellisense\\MSVCExternalInclude\\Space\\Path" );
            TEST_ASSERT( includes[ 34 ] == "Intellisense\\MSVCExternalInclude\\Slash\\Path" );
            TEST_ASSERT( includes[ 35 ] == "Intellisense\\MSVCExternalInclude\\Slash\\Space\\Path" );
            TEST_ASSERT( includes[ 36 ] == "Intellisense\\MSVCExternalInclude\\Quoted\\Path" );
            TEST_ASSERT( includes[ 37 ] == "Intellisense\\MSVCExternalInclude\\Quoted\\Space\\Path" );
            TEST_ASSERT( includes[ 38 ] == "Intellisense\\MSVCExternalInclude\\Quoted\\Slash\\Path" );
            TEST_ASSERT( includes[ 39 ] == "Intellisense\\MSVCExternalInclude\\Quoted\\Slash\\Space\\Path" );

            includesOk = true;
        }
        else if ( token.Find( "AdditionalOptions" ) )
        {
            TEST_ASSERT( token.Find( "-std:c++17" ) );
            TEST_ASSERT( token.Find( "/std:c++14" ) );
            TEST_ASSERT( token.Find( "/std:latest" ) );
            additionalOptionsOk = true;
        }
    }
    TEST_ASSERT( definesOk );
    TEST_ASSERT( includesOk );
    TEST_ASSERT( additionalOptionsOk );
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
    bool definesOk[ NUM_DEFINES ] = {};
    Array< AString > includes;
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
            // We only interested in checking relative ordering of paths that contain "Intellisense\\".
            const char * pathStartPos = token.Find( "Intellisense/" );
            if ( pathStartPos != nullptr )
            {

                // Check that we separated path from the option name correctly.
                TEST_ASSERT( ( pathStartPos == token.Get() ) || ( pathStartPos[ -1 ] == '/' ) );

                const char * pathEndPos = token.GetEnd() - ( token.EndsWith( ',' ) ? 1 : 0 );
                includes.EmplaceBack( pathStartPos, pathEndPos );
            }
            continue;
        }
    }

    // Check we found them all
    for ( size_t i = 0; i < NUM_DEFINES; ++i )
    {
        TEST_ASSERT( definesOk[ i ] );
    }

    TEST_ASSERT( includes.GetSize() == 40 );
    TEST_ASSERT( includes[  0 ] == "Intellisense/Include/Path" );
    TEST_ASSERT( includes[  1 ] == "Intellisense/Include/Space/Path" );
    TEST_ASSERT( includes[  2 ] == "Intellisense/Include/Slash/Path" );
    TEST_ASSERT( includes[  3 ] == "Intellisense/Include/Slash/Space/Path" );
    TEST_ASSERT( includes[  4 ] == "Intellisense/Include/Quoted/Path" );
    TEST_ASSERT( includes[  5 ] == "Intellisense/Include/Quoted/Space/Path" );
    TEST_ASSERT( includes[  6 ] == "Intellisense/Include/Quoted/Slash/Path" );
    TEST_ASSERT( includes[  7 ] == "Intellisense/Include/Quoted/Slash/Space/Path" );
    TEST_ASSERT( includes[  8 ] == "Intellisense/SystemInclude/Path" );
    TEST_ASSERT( includes[  9 ] == "Intellisense/SystemInclude/Space/Path" );
    TEST_ASSERT( includes[ 10 ] == "Intellisense/SystemInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 11 ] == "Intellisense/SystemInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 12 ] == "Intellisense/SystemAfterInclude/Path" );
    TEST_ASSERT( includes[ 13 ] == "Intellisense/SystemAfterInclude/Space/Path" );
    TEST_ASSERT( includes[ 14 ] == "Intellisense/SystemAfterInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 15 ] == "Intellisense/SystemAfterInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 16 ] == "Intellisense/MSVCInclude/Path" );
    TEST_ASSERT( includes[ 17 ] == "Intellisense/MSVCInclude/Space/Path" );
    TEST_ASSERT( includes[ 18 ] == "Intellisense/MSVCInclude/Slash/Path" );
    TEST_ASSERT( includes[ 19 ] == "Intellisense/MSVCInclude/Slash/Space/Path" );
    TEST_ASSERT( includes[ 20 ] == "Intellisense/MSVCInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 21 ] == "Intellisense/MSVCInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 22 ] == "Intellisense/MSVCInclude/Quoted/Slash/Path" );
    TEST_ASSERT( includes[ 23 ] == "Intellisense/MSVCInclude/Quoted/Slash/Space/Path" );
    TEST_ASSERT( includes[ 24 ] == "Intellisense/DirAfterInclude/Path" );
    TEST_ASSERT( includes[ 25 ] == "Intellisense/DirAfterInclude/Space/Path" );
    TEST_ASSERT( includes[ 26 ] == "Intellisense/DirAfterInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 27 ] == "Intellisense/DirAfterInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 28 ] == "Intellisense/QuoteInclude/Path" );
    TEST_ASSERT( includes[ 29 ] == "Intellisense/QuoteInclude/Space/Path" );
    TEST_ASSERT( includes[ 30 ] == "Intellisense/QuoteInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 31 ] == "Intellisense/QuoteInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 32 ] == "Intellisense/MSVCExternalInclude/Path" );
    TEST_ASSERT( includes[ 33 ] == "Intellisense/MSVCExternalInclude/Space/Path" );
    TEST_ASSERT( includes[ 34 ] == "Intellisense/MSVCExternalInclude/Slash/Path" );
    TEST_ASSERT( includes[ 35 ] == "Intellisense/MSVCExternalInclude/Slash/Space/Path" );
    TEST_ASSERT( includes[ 36 ] == "Intellisense/MSVCExternalInclude/Quoted/Path" );
    TEST_ASSERT( includes[ 37 ] == "Intellisense/MSVCExternalInclude/Quoted/Space/Path" );
    TEST_ASSERT( includes[ 38 ] == "Intellisense/MSVCExternalInclude/Quoted/Slash/Path" );
    TEST_ASSERT( includes[ 39 ] == "Intellisense/MSVCExternalInclude/Quoted/Slash/Space/Path" );
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
    TEST_ASSERT( fBuild.Build( "DefaultConfigs" ) );

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
    TEST_ASSERT( fBuild.Build( "PerConfigOverrides" ) );

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
    // TODO:C This test adds paths that are not normalized, but project nodes
    // should only be passing in normalized paths
    #if defined( __WINDOWS__ )
        pg.AddFile( AStackString<>( "C:\\Code\\File.cpp" ) );
        pg.AddFile( AStackString<>( "C:\\Code\\file.cpp" ) );               // Duplicate with case difference
        pg.AddFile( AStackString<>( "C:\\Code\\File.cpp" ) );               // Exact duplicate
        pg.AddFile( AStackString<>( "C:\\Code\\../Code/File.cpp" ) );       // Duplicate with path difference
        pg.AddFile( AStackString<>( "C:\\Code\\../Dir/../Code/File.cpp" ) );// Duplicate with path difference
        AStackString<> projectFileName( "C:\\Code\\dummy.vcxproj" );
    #else
        pg.AddFile( AStackString<>( "/Code/File.cpp" ) );
        pg.AddFile( AStackString<>( "/Code/file.cpp" ) );                   // Duplicate with case difference
        pg.AddFile( AStackString<>( "/Code/File.cpp" ) );                   // Exact duplicate
        pg.AddFile( AStackString<>( "/Code/../Code/File.cpp" ) );           // Duplicate with path difference
        pg.AddFile( AStackString<>( "/Code/../Dir/../Code/File.cpp" ) );    // Duplicate with path difference
        AStackString<> projectFileName( "/Code/dummy.vcxproj" );
    #endif

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >(), Array< VSProjectImport >() ) );
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
    #if defined( __WINDOWS__ )
        AStackString<> basePath( "C:\\" );

        // Files in various sub-dirs
        AStackString<> file1( "C:\\FolderA\\AFile.cpp" );
        AStackString<> file2( "C:\\FolderA\\BFolder\\SubDir\\AFile.cpp" );
        AStackString<> file3( "C:\\FolderA\\ZFile.cpp" );
        AStackString<> file4( "C:\\FolderA\\ZFolder\\SubDir\\AFile.cpp" );
        AStackString<> file5( "C:\\FolderZ\\ZFile.cpp" );

        // Dirs which are substrings of each other but unique
        AStackString<> file6( "C:\\Data\\TestPrecompiledHeaders\\CacheUniqueness2\\PrecompiledHeader.cpp" );
        AStackString<> file7( "C:\\Data\\TestPrecompiledHeaders\\CacheUniqueness\\PrecompiledHeader.cpp" );

        // Project name
        AStackString<> projectFileName( "C:\\dummy.vcxproj" );
    #else
        AStackString<> basePath( "/" );

        // Files in various sub-dirs
        AStackString<> file1( "/FolderA/AFile.cpp" );
        AStackString<> file2( "/FolderA/BFolder/SubDir/AFile.cpp" );
        AStackString<> file3( "/FolderA/ZFile.cpp" );
        AStackString<> file4( "/FolderA/ZFolder/SubDir/AFile.cpp" );
        AStackString<> file5( "/FolderZ/ZFile.cpp" );

        // Dirs which are substrings of each other but unique
        AStackString<> file6( "/Data/TestPrecompiledHeaders/CacheUniqueness2/PrecompiledHeader.cpp" );
        AStackString<> file7( "/Data/TestPrecompiledHeaders/CacheUniqueness/PrecompiledHeader.cpp" );

        // Project name
        AStackString<> projectFileName( "/dummy.vcxproj" );
    #endif

    FBuild fb; // For CleanPath

    VSProjectGenerator pg;

    // Project name
    AStackString<> name( "Project" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );

    // Base dir
    Array< AString > basePaths;
    basePaths.Append( basePath );
    pg.SetBasePaths( basePaths );

    // Platforms
    Array< VSProjectConfig > configs;
    configs.SetCapacity( 6 );
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // Files
    pg.AddFile( file1 );
    pg.AddFile( file2 );
    pg.AddFile( file3 );
    pg.AddFile( file4 );
    pg.AddFile( file5 );
    pg.AddFile( file6 );
    pg.AddFile( file7 );

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >(), Array< VSProjectImport >() ) );
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

// VCXProj_ProjectRelativePaths
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_ProjectRelativePaths() const
{
    // Overlapping input and output directories, with common substring in dir names
    #if defined( __WINDOWS__ )
        AStackString<> basePath       ( "C:\\MyProject\\ProjectSourceFiles\\" );
        AStackString<> fileA          ( "C:\\MyProject\\ProjectSourceFiles\\File.cpp" );
        AStackString<> fileB          ( "C:\\MyProject\\ProjectSourceFiles\\SubDir\\File.cpp" );
        AStackString<> projectFileName( "C:\\MyProject\\Projects\\MyProject.vcxproj" );
        //                                              ^     ^
        //                                              \-----\-- NOTE partial overlap within dir name
    #else
        AStackString<> basePath       ( "/MyProject/ProjectSourceFiles/" );
        AStackString<> fileA          ( "/MyProject/ProjectSourceFiles/File.cpp" );
        AStackString<> fileB          ( "/MyProject/ProjectSourceFiles/SubDir/File.cpp" );
        AStackString<> projectFileName( "/MyProject/Projects/MyProject.vcxproj" );
        //                                          ^     ^
        //                                          \-----\-- NOTE partial overlap within dir name
    #endif

    FBuild fb; // For CleanPath

    VSProjectGenerator pg;

    // Project name
    AStackString<> name( "Project" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );

    // Base dir
    Array< AString > basePaths;
    basePaths.Append( basePath );
    pg.SetBasePaths( basePaths );

    // Platforms
    Array< VSProjectConfig > configs;
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // Files
    pg.AddFile( fileA );
    pg.AddFile( fileB );

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >(), Array< VSProjectImport >() ) );
        TEST_ASSERT( proj.Replace( "<CustomBuild Include=\"..\\ProjectSourceFiles\\File.cpp\" />", "" ) == 1 );
        TEST_ASSERT( proj.Replace( "<CustomBuild Include=\"..\\ProjectSourceFiles\\SubDir\\File.cpp\" />", "" ) == 1 );
        TEST_ASSERT( proj.FindI( "<CustomBuild " ) == nullptr );
    }

    // Check vcxproj.filters
    {
        AStackString<> filter( pg.GenerateVCXProjFilters( projectFileName ) );

        // Should have an entry for each file
        TEST_ASSERT( filter.Replace( "<CustomBuild Include=\"..\\ProjectSourceFiles\\File.cpp\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<CustomBuild Include=\"..\\ProjectSourceFiles\\SubDir\\File.cpp\">", "" ) == 1 );
        TEST_ASSERT( filter.FindI( "<CustomBuild " ) == nullptr );

        // Ensure test has accounted for all paths - should be exactly one
        TEST_ASSERT( filter.Replace( "<Filter Include=\"SubDir\">", "" ) == 1 );
        TEST_ASSERT( filter.Find( "<Filter Include=" ) == nullptr );
    }
}

// VCXProj_ProjectRelativePaths2
//------------------------------------------------------------------------------
void TestProjectGeneration::VCXProj_ProjectRelativePaths2() const
{
    // Overlapping input and output directories, with source files in same
    // dir as .vcxproject which is a sub-dir of a basepath
    #if defined( __WINDOWS__ )
        AStackString<> basePath       ( "C:\\MyProject\\" );
        AStackString<> fileA          ( "C:\\MyProject\\Generated\\GeneratedCpp.cpp" );
        AStackString<> fileB          ( "C:\\MyProject\\Generated\\SubDir\\GeneratedCpp.cpp" );
        AStackString<> projectFileName( "C:\\MyProject\\Generated\\MyProject.vcxproj" );
        //                                              ^       ^
        //                                              \-------\-- NOTE common dir name
    #else
        AStackString<> basePath       ( "/MyProject/" );
        AStackString<> fileA          ( "/MyProject/Generated/GeneratedCpp.cpp" );
        AStackString<> fileB          ( "/MyProject/Generated/SubDir/GeneratedCpp.cpp" );
        AStackString<> projectFileName( "/MyProject/Generated/MyProject.vcxproj" );
        //                                          ^       ^
        //                                          \-------\-- NOTE common dir name
    #endif

    FBuild fb; // For CleanPath

    VSProjectGenerator pg;

    // Project name
    AStackString<> name( "Project" );
    AStackString<> guid;
    VSProjectGenerator::FormatDeterministicProjectGUID( guid, name );
    pg.SetProjectGuid( guid );

    // Base dir
    Array< AString > basePaths;
    basePaths.Append( basePath );
    pg.SetBasePaths( basePaths );

    // Platforms
    Array< VSProjectConfig > configs;
    VSProjectConfig cfg;
    cfg.m_Platform = "Win32";
    cfg.m_Config = "Debug";
    configs.Append( cfg );

    // Files
    pg.AddFile( fileA );
    pg.AddFile( fileB );

    // Check vcxproj
    {
        AStackString<> proj( pg.GenerateVCXProj( projectFileName, configs, Array< VSProjectFileType >(), Array< VSProjectImport >() ) );
        TEST_ASSERT( proj.Replace( "<CustomBuild Include=\"GeneratedCpp.cpp\" />", "" ) == 1 );
        TEST_ASSERT( proj.Replace( "<CustomBuild Include=\"SubDir\\GeneratedCpp.cpp\" />", "" ) == 1 );
        TEST_ASSERT( proj.FindI( "<CustomBuild " ) == nullptr );
    }

    // Check vcxproj.filters
    {
        AStackString<> filter( pg.GenerateVCXProjFilters( projectFileName ) );

        // Should have an entry for each file
        TEST_ASSERT( filter.Replace( "<CustomBuild Include=\"GeneratedCpp.cpp\">", "" ) == 1 ); // File path is still project-relative
        TEST_ASSERT( filter.Replace( "<Filter>Generated</Filter>", "" ) == 1 );                 // File will appear in this folder
        TEST_ASSERT( filter.Replace( "<CustomBuild Include=\"SubDir\\GeneratedCpp.cpp\">", "" ) == 1 ); // File path is still project-relative
        TEST_ASSERT( filter.Replace( "<Filter>Generated\\SubDir</Filter>", "" ) == 1 );                 // File will appear in this folder
        TEST_ASSERT( filter.FindI( "<CustomBuild " ) == nullptr );

        // Ensure test has accounted for all paths - should be exactly one
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Generated\">", "" ) == 1 );
        TEST_ASSERT( filter.Replace( "<Filter Include=\"Generated\\SubDir\">", "" ) == 1 );
        TEST_ASSERT( filter.Find( "<Filter Include=" ) == nullptr );
    }
}

// Solution_Empty
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_Empty() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_Empty/empty.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_Empty/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "EmptySolution" ) );

    //
    EnsureFileExists( solution );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::SLN_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     2 );
}

// Solution_SolutionRelativePaths
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_SolutionRelativePaths() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_SolutionRelativePaths/SubDir2/solution.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_SolutionRelativePaths/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "Solution" ) );

    //
    EnsureFileExists( solution );

    // Read the project into memory
    FileStream f;
    VERIFY( f.Open( solution.Get(), FileStream::READ_ONLY ) );
    AStackString<> solutionData;
    solutionData.SetLength( (uint32_t)f.GetFileSize() );
    VERIFY( f.ReadBuffer( solutionData.Get(), solutionData.GetLength() ) == solutionData.GetLength() );

    // Ensure the path to the project is relative and not absolute
    TEST_ASSERT( solutionData.Find( "\"proj1\", \"..\\SubDir1\\proj1.vcxproj\"," ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::VCXPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::SLN_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     3 );
}

// Solution_BuildAndDeploy_None
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_BuildAndDeploy_None() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_BuildAndDeploy_None/solution.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_BuildAndDeploy_None/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "Solution" ) );

    //
    EnsureFileExists( solution );

    // Read the project into memory
    FileStream f;
    VERIFY( f.Open( solution.Get(), FileStream::READ_ONLY ) );
    AStackString<> solutionData;
    solutionData.SetLength( (uint32_t)f.GetFileSize() );
    VERIFY( f.ReadBuffer( solutionData.Get(), solutionData.GetLength() ) == solutionData.GetLength() );

    // Ensure no projects is set to Build or Deploy
    TEST_ASSERT( solutionData.Find( ".Build" ) == nullptr );
    TEST_ASSERT( solutionData.Find( ".Deploy" ) == nullptr );
}

// Solution_BuildAndDeploy_Project
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_BuildAndDeploy_Project() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_BuildAndDeploy_Project/solution.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_BuildAndDeploy_Project/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "Solution" ) );

    //
    EnsureFileExists( solution );

    // Read the project into memory
    FileStream f;
    VERIFY( f.Open( solution.Get(), FileStream::READ_ONLY ) );
    AStackString<> solutionData;
    solutionData.SetLength( (uint32_t)f.GetFileSize() );
    VERIFY( f.ReadBuffer( solutionData.Get(), solutionData.GetLength() ) == solutionData.GetLength() );

    // Ensure one project is set to Build, for all 4 configs
    TEST_ASSERT( solutionData.Replace( "Debug|x64.Build.0 = Debug|x64", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Debug|x86.Build.0 = Debug|Win32", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Release|x64.Build.0 = Release|x64", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Release|x86.Build.0 = Release|Win32", "" ) == 1 );

    // Ensure one project is set to Deploy, for all 4 configs
    TEST_ASSERT( solutionData.Replace( "Debug|x64.Deploy.0 = Debug|x64", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Debug|x86.Deploy.0 = Debug|Win32", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Release|x64.Deploy.0 = Release|x64", "" ) == 1 );
    TEST_ASSERT( solutionData.Replace( "Release|x86.Deploy.0 = Release|Win32", "" ) == 1 );

    // Ensure no other unexpected Build/Deploy settings are written
    TEST_ASSERT( solutionData.Find( ".Build." ) == nullptr );
    TEST_ASSERT( solutionData.Find( ".Deploy." ) == nullptr );
}

// Solution_BuildAndDeploy_PerSolutionConfig
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_BuildAndDeploy_PerSolutionConfig() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_BuildAndDeploy_PerSolutionConfig/solution.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_BuildAndDeploy_PerSolutionConfig/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "Solution" ) );

    //
    EnsureFileExists( solution );

    // Read the project into memory
    FileStream f;
    VERIFY( f.Open( solution.Get(), FileStream::READ_ONLY ) );
    AStackString<> solutionData;
    solutionData.SetLength( (uint32_t)f.GetFileSize() );
    VERIFY( f.ReadBuffer( solutionData.Get(), solutionData.GetLength() ) == solutionData.GetLength() );

    // Ensure one project is set to Build, only for one config
    TEST_ASSERT( solutionData.Replace( "Debug|x86.Build.0 = Debug|Win32", "" ) == 1 );

    // Ensure one project is set to Deploy, only for one config
    TEST_ASSERT( solutionData.Replace( "Release|x64.Deploy.0 = Release|x64", "" ) == 1 );

    // Ensure no other unexpected Build/Deploy settings are written
    TEST_ASSERT( solutionData.Find( ".Build." ) == nullptr );
    TEST_ASSERT( solutionData.Find( ".Deploy." ) == nullptr );
}

// Solution_Items
//------------------------------------------------------------------------------
void TestProjectGeneration::Solution_Items() const
{
    const AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_Items/solution_with_items.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_Items/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "Solution_with_items" ) );

    //
    EnsureFileExists( solution );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode(  1,     1,      Node::SLN_NODE );
    CheckStatsNode(  1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 6,     6 );
}


// VSExternalProj_ExternalProject
//------------------------------------------------------------------------------
void TestProjectGeneration::VSExternalProj_ExternalProject() const
{
    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_ExternalProject/External.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_ExternalProject/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // do build
    TEST_ASSERT( fBuild.Build( "ExternalProjectSolution" ) );

    //
    EnsureFileExists( solution );

    // Check stats
    //              Seen,   Built,  Type
    CheckStatsNode( 1,      1,      Node::SLN_NODE );
    CheckStatsNode( 1,      1,      Node::ALIAS_NODE );

    // because of the external module, peek how many of them were actually processed, depending if using the module is
    // enforced or not in the actual fbuild.bff 
    const FBuildStats& stats = FBuild::Get().GetStats();
    const FBuildStats::Stats& nodeStatsExternal = stats.GetStatsFor( Node::VSPROJEXTERNAL_NODE );
    const size_t actualNumExtSeen = nodeStatsExternal.m_NumProcessed;
    const size_t actualNumExtBuilt = nodeStatsExternal.m_NumBuilt;

    CheckStatsTotal( 4 + actualNumExtSeen, 4 + actualNumExtBuilt );
}

// VSExternalProj_ExternalProjectWrongData
//------------------------------------------------------------------------------
void TestProjectGeneration::VSExternalProj_ExternalProjectWrongData() const
{
    // this test really needs to use the external module on a wrong project, in order to validate a failure scenario
    // therefore it should only be ever run on windows, with properly installed VS

    AStackString<> solution( "../tmp/Test/ProjectGeneration/Solution_ExternalProject/ExternalWrongData.sln" );

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_ExternalProject/fbuild_WrongData.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Delete old files from previous runs
    EnsureFileDoesNotExist( solution );

    // building will fail
    TEST_ASSERT( fBuild.Build( "ExternalWrongDataProjectSolution" ) == false );

    CheckStatsTotal( 5, 2 );
}

// VSExternalProj_ExternalProject_MissingProjectGUID
//------------------------------------------------------------------------------
void TestProjectGeneration::VSExternalProj_ExternalProject_MissingProjectGUID() const
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestProjectGeneration/Solution_ExternalProject/fbuild_MissingProjectGUID.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // building will fail
    TEST_ASSERT( fBuild.Build( "ExternalProject_MissingProjectGUID" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Failed to extract <ProjectGuid>" ) );
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
    TEST_ASSERT( fBuild.Build( "XCodeProj" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 0,     0,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::XCODEPROJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     2 );
}

//------------------------------------------------------------------------------
