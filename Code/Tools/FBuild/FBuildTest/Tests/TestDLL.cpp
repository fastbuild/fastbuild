// TestBuildAndLinkLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestBuildAndLinkLibrary
//------------------------------------------------------------------------------
class TestDLL : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestSingleDLL() const;
    void TestSingleDLL_NoRebuild() const;
    void TestTwoDLLs() const;
    void TestTwoDLLs_NoRebuild() const;
    void TestTwoDLLs_NoUnnecessaryRelink() const;
    void TestDLLWithPCH() const;
    void TestDLLWithPCH_NoRebuild() const;
    void TestExeWithDLL() const;
    void TestExeWithDLL_NoRebuild() const;
    void TestValidExeWithDLL() const;

    void TestLinkWithCopy() const;

    const char * GetSingleDLLDBFileName() const { return "../../../../tmp/Test/DLL/singledll.fdb"; }
    const char * GetTwoDLLsDBFileName() const   { return "../../../../tmp/Test/DLL/twodlls.fdb"; }
    const char * GetDLLWithPCHDBFileName() const { return "../../../../tmp/Test/DLL/dllwithpch.fdb"; }
    const char * GetExeWithDLLDBFileName() const { return "../../../../tmp/Test/DLL/dllwithexe.fdb"; }
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestDLL )
    REGISTER_TEST( TestSingleDLL )
    REGISTER_TEST( TestSingleDLL_NoRebuild )
    REGISTER_TEST( TestTwoDLLs )
    REGISTER_TEST( TestTwoDLLs_NoRebuild )
    REGISTER_TEST( TestTwoDLLs_NoUnnecessaryRelink )
    REGISTER_TEST( TestDLLWithPCH )
    REGISTER_TEST( TestDLLWithPCH_NoRebuild )
    REGISTER_TEST( TestExeWithDLL )
    REGISTER_TEST( TestExeWithDLL_NoRebuild )
    REGISTER_TEST( TestValidExeWithDLL )
    REGISTER_TEST( TestLinkWithCopy )
REGISTER_TESTS_END

// TestSingleDLL
//------------------------------------------------------------------------------
void TestDLL::TestSingleDLL() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dll( "../tmp/Test/DLL/dll.dll" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dll );

    TEST_ASSERT( fBuild.Build( dll ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetSingleDLLDBFileName() ) );

    // make sure all output files are as expecter
    EnsureFileExists( dll );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::DLL_NODE );
}

// TestSingleDLL_NoRebuild
//------------------------------------------------------------------------------
void TestDLL::TestSingleDLL_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetSingleDLLDBFileName() ) );

    const AStackString<> dll( "../tmp/Test/DLL/dll.dll" );

    TEST_ASSERT( fBuild.Build( dll ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     0,      Node::DLL_NODE );
}

// TestTwoDLLs
//------------------------------------------------------------------------------
void TestDLL::TestTwoDLLs() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest//Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dllA( "../tmp/Test/DLL/dllA.dll" );
    const AStackString<> dllB( "../tmp/Test/DLL/dllB.dll" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dllA );
    EnsureFileDoesNotExist( dllB );

    // build dllB which depends on dllA
    TEST_ASSERT( fBuild.Build( dllB ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTwoDLLsDBFileName() ) );

    // make sure all output files are as expecter
    EnsureFileExists( dllA );
    EnsureFileExists( dllB );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 2,     2,      Node::DLL_NODE );
}

// TestTwoDLLs_NoRebuild
//------------------------------------------------------------------------------
void TestDLL::TestTwoDLLs_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetTwoDLLsDBFileName() ) );

    // build again
    const AStackString<> dllB( "../tmp/Test/DLL/dllB.dll" );
    TEST_ASSERT( fBuild.Build( dllB ) );

    // Check stats to be sure nothing was built
    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 2,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 2,     0,      Node::DLL_NODE );
}

// TestTwoDLLs_NoUnnecessaryRelink
//------------------------------------------------------------------------------
void TestDLL::TestTwoDLLs_NoUnnecessaryRelink() const
{
    // 1) Force DLL A to build
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( GetTwoDLLsDBFileName() ) );

        const AStackString<> dllA( "../tmp/Test/DLL/dllA.dll" );

        // delete DLL A to have it relink (and regen the import lib)
        EnsureFileDoesNotExist( dllA );
        TEST_ASSERT( fBuild.Build( dllA ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( GetTwoDLLsDBFileName() ) );

        // Check stats to be sure one dll was built
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
        CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::DLL_NODE );
    }

    // 2) Ensure DLL B does not relink
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( GetTwoDLLsDBFileName() ) );

        // build again
        const AStackString<> dllB( "../tmp/Test/DLL/dllB.dll" );
        TEST_ASSERT( fBuild.Build( dllB ) );

        // Check stats to be sure nothing was built
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
        CheckStatsNode ( 2,     0,      Node::OBJECT_NODE );
        CheckStatsNode ( 2,     0,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 2,     0,      Node::DLL_NODE );
    }
}

// TestDLLWithPCH
//------------------------------------------------------------------------------
void TestDLL::TestDLLWithPCH() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dllPCH( "../tmp/Test/DLL/dllPCH.dll" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dllPCH );

    // build dllB which depends on dllA
    TEST_ASSERT( fBuild.Build( dllPCH ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetDLLWithPCHDBFileName() ) );

    // make sure all output files are as expecter
    EnsureFileExists( dllPCH );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::DLL_NODE );
}

// TestDLLWithPCH_NoRebuild
//------------------------------------------------------------------------------
void TestDLL::TestDLLWithPCH_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetDLLWithPCHDBFileName() ) );

    const AStackString<> dllPCH( "../tmp/Test/DLL/dllPCH.dll" );

    // build dllB which depends on dllA
    TEST_ASSERT( fBuild.Build( dllPCH ) );

    // Check we build what was expected
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     0,      Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     0,      Node::DLL_NODE );
}

// TestExeWithDLL
//------------------------------------------------------------------------------
void TestDLL::TestExeWithDLL() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> exe( "../tmp/Test/DLL/exe.exe" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( exe );

    // build executable with depends on DLLA
    TEST_ASSERT( fBuild.Build( exe ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetExeWithDLLDBFileName() ) );

    // make sure all output files are as expecter
    EnsureFileExists( exe );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );    // exe.obj + a.obj
    CheckStatsNode ( 2,     2,      Node::OBJECT_LIST_NODE );   // exe lib + dll lib
    CheckStatsNode ( 1,     1,      Node::DLL_NODE );
    CheckStatsNode ( 1,     1,      Node::EXE_NODE );
}

// TestExeWithDLL_NoRebuild
//------------------------------------------------------------------------------
void TestDLL::TestExeWithDLL_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetExeWithDLLDBFileName() ) );

    // build executable with depends on DLLA
    TEST_ASSERT( fBuild.Build( "../tmp/Test/DLL/exe.exe" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     0,      Node::OBJECT_NODE );    // exe.obj + a.obj
    CheckStatsNode ( 2,     0,      Node::OBJECT_LIST_NODE );   // exe lib + dll lib
    CheckStatsNode ( 1,     0,      Node::DLL_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
}

// TestValidExeWithDLL
//------------------------------------------------------------------------------
void TestDLL::TestValidExeWithDLL() const
{
    const AStackString<> exe( "../tmp/Test/DLL/exe.exe" );

    Process p;
    p.Spawn( exe.Get(), nullptr, nullptr, nullptr );
    int ret = p.WaitForExit();
    TEST_ASSERT( ret == 99 );
}

// TestLinkWithCopy
//------------------------------------------------------------------------------
void TestDLL::TestLinkWithCopy() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDLL/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build executable with depends on DLLA
    TEST_ASSERT( fBuild.Build( "DllBUsingCopy" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 2,     2,      Node::DLL_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
}

//------------------------------------------------------------------------------
