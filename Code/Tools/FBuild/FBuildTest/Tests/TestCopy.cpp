// TestCopy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestCopy
//------------------------------------------------------------------------------
class TestCopy : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestCopyFunction_FileToFile() const;
    void TestCopyFunction_FileToFile_NoRebuild() const;
    void TestCopyFunction_FileToDir() const;
    void TestCopyFunction_FileToDir_NoRebuild() const;
    void TestCopyFunction_MultiFileToDir() const;
    void TestCopyFunction_MultiFileToDir_NoRebuild() const;
    void TestCopyFunction_SourceBasePath() const;
    void TestCopyFunction_SourceBasePath_NoRebuild() const;
    void ChainedCopy() const;
    void ChainedCopy_NoRebuild() const;
    void CopyDir() const;
    void CopyDir_NoRebuild() const;
    void CopyDirDeleteSrc() const;
    void CopyEmpty() const;
    void MissingTrailingSlash() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCopy )
    REGISTER_TEST( TestCopyFunction_FileToFile )
    REGISTER_TEST( TestCopyFunction_FileToFile_NoRebuild )
    REGISTER_TEST( TestCopyFunction_FileToDir )
    REGISTER_TEST( TestCopyFunction_FileToDir_NoRebuild )
    REGISTER_TEST( TestCopyFunction_MultiFileToDir )
    REGISTER_TEST( TestCopyFunction_MultiFileToDir_NoRebuild )
    REGISTER_TEST( TestCopyFunction_SourceBasePath )
    REGISTER_TEST( TestCopyFunction_SourceBasePath_NoRebuild )
    REGISTER_TEST( ChainedCopy )
    REGISTER_TEST( ChainedCopy_NoRebuild )
    REGISTER_TEST( CopyDir )
    REGISTER_TEST( CopyDir_NoRebuild )
    REGISTER_TEST( CopyDirDeleteSrc )
    REGISTER_TEST( CopyEmpty )
    REGISTER_TEST( MissingTrailingSlash )
REGISTER_TESTS_END

// TestCopyFunction_FileToFile
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToFile() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString<> dst( "../../../../tmp/Test/Copy/copy.bff.copy" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToFile" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/filetofile.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dst );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     3 );
}

// TestCopyFunction_FileToFile_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToFile_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/filetofile.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToFile" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 1,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     2 );
}

// TestCopyFunction_FileToDir
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToDir() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dst( "../../../../tmp/Test/Copy/copy.bff" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToDir" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/filetodir.fdb" ) );

    EnsureFileExists( dst );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     3 );
}

// TestCopyFunction_FileToDir_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToDir_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/filetodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToDir" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 1,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 3,     2 );
}

// TestCopyFunction_MultiFileToDir
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_MultiFileToDir() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dst1( "../../../../tmp/Test/Copy/a.txt" );
    const AStackString<> dst2( "../../../../tmp/Test/Copy/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst1 );
    EnsureFileDoesNotExist( dst2 );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestMultiCopyToDir" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/multifiletodir.fdb" ) );

    EnsureFileExists( dst1 );
    EnsureFileExists( dst2 );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     2,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     5 );
}

// TestCopyFunction_MultiFileToDir_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_MultiFileToDir_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/multifiletodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestMultiCopyToDir" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     3 );
}

// TestCopyFunction_SourceBasePath
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_SourceBasePath() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> dst1( "../../../../tmp/Test/Copy/SourceBasePath/TestCopy/a.txt" );
    const AStackString<> dst2( "../../../../tmp/Test/Copy/SourceBasePath/TestCopy/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst1 );
    EnsureFileDoesNotExist( dst2 );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestSourceBasePath" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/SourceBasePath/sourcebasepath.fdb" ) );

    EnsureFileExists( dst1 );
    EnsureFileExists( dst2 );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     2,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     5 );
}

// TestCopyFunction_SourceBasePath_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_SourceBasePath_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/SourceBasePath/sourcebasepath.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "TestSourceBasePath" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     3 );
}

// ChainedCopy
//------------------------------------------------------------------------------
void TestCopy::ChainedCopy() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString<> dst( "../../../../tmp/Test/Copy/ChainedCopy/copy.copy3" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "ChainedCopy3" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/ChainedCopy/chainedcopy.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dst );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 3,     3,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     5 );
}

// ChainedCopy_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::ChainedCopy_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/ChainedCopy/chainedcopy.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "ChainedCopy3" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 3,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 5,     2 );
}

// CopyDir
//------------------------------------------------------------------------------
void TestCopy::CopyDir() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString<> dstA( "../../../../tmp/Test/Copy/CopyDir/a.txt" );
    AStackString<> dstB( "../../../../tmp/Test/Copy/CopyDir/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dstA );
    EnsureFileDoesNotExist( dstB );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CopyDir" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/CopyDir/copydir.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dstA );
    EnsureFileExists( dstB );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     2,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::COPY_DIR_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 6,     6 );
}

// CopyDir_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::CopyDir_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/CopyDir/copydir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CopyDir" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );
    CheckStatsNode ( 2,     0,      Node::COPY_FILE_NODE );
    CheckStatsNode ( 1,     0,      Node::COPY_DIR_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 6,     3 );
}

// CopyDirDeleteSrc
//------------------------------------------------------------------------------
void TestCopy::CopyDirDeleteSrc() const
{
    // We'll operate on copies of the source files so we can delete on of the src files
    AStackString<> srcA( "../../../../tmp/Test/Copy/CopyDirDeleteSrc/Src/a.txt" );
    AStackString<> srcB( "../../../../tmp/Test/Copy/CopyDirDeleteSrc/Src/b.txt" );
    AStackString<> dstA( "../../../../tmp/Test/Copy/CopyDirDeleteSrc/Dst/a.txt" );
    AStackString<> dstB( "../../../../tmp/Test/Copy/CopyDirDeleteSrc/Dst/b.txt" );

    // Set up the source
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString<>( "../../../../tmp/Test/Copy/CopyDirDeleteSrc/Src/" ) ) );
    TEST_ASSERT( FileIO::FileCopy( "Data/TestCopy/a.txt", srcA.Get() ) );
    TEST_ASSERT( FileIO::FileCopy( "Data/TestCopy/b.txt", srcB.Get() ) );
    TEST_ASSERT( FileIO::SetReadOnly( srcA.Get(), false ) ); // Clear read only so it's not persisted by copy
    TEST_ASSERT( FileIO::SetReadOnly( srcB.Get(), false ) ); // Clear read only so it's not persisted by copy

    // Do the normal build, which copies the files
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestCopy/copy.bff";
        options.m_ShowSummary = true; // required to generate stats for node count checks
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // clean up anything left over from previous runs
        EnsureFileDoesNotExist( dstA );
        EnsureFileDoesNotExist( dstB );

        // build (via alias)
        TEST_ASSERT( fBuild.Build( AStackString<>( "CopyDirDeleteSrc" ) ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/Copy/CopyDir/copydirdeletesrc.fdb" ) );

        // make sure all output is where it is expected
        EnsureFileExists( dstA );
        EnsureFileExists( dstB );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode(  2,     2,      Node::FILE_NODE );
        CheckStatsNode(  2,     2,      Node::COPY_FILE_NODE );
        CheckStatsNode(  1,     1,      Node::COPY_DIR_NODE );
        CheckStatsNode(  1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsTotal( 6,     6 );
    }

    // Delete one of the source files
    EnsureFileDoesNotExist( srcB );

    // Build again to make sure it doesn't fail
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestCopy/copy.bff";
        options.m_ShowSummary = true; // required to generate stats for node count checks
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/Copy/CopyDir/copydirdeletesrc.fdb" ) );

        // build (via alias)
        TEST_ASSERT( fBuild.Build( AStackString<>( "CopyDirDeleteSrc" ) ) );

        // make sure all output is where it is expected
        EnsureFileExists( dstA );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode(  1,     1,      Node::FILE_NODE );
        CheckStatsNode(  1,     0,      Node::COPY_FILE_NODE );
        CheckStatsNode(  1,     0,      Node::COPY_DIR_NODE );
        CheckStatsNode(  1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsTotal( 4,     2 );
    }
}

// CopyEmpty
//------------------------------------------------------------------------------
void TestCopy::CopyEmpty() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CopyEmpty" ) ) );
}

// MissingTrailingSlash
//------------------------------------------------------------------------------
void TestCopy::MissingTrailingSlash() const
{
    // Ensure a copy of multiple files to destination that is not a path
    // is reported as an explicit error
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCopy/MissingTrailingSlash/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );

    // Parsing should fail
    TEST_ASSERT( fBuild.Initialize() == false );

    // Ensure the explcit error for this case is reported (not a generic one about
    // the target already being defined)
    TEST_ASSERT( GetRecordedOutput().Find( "FASTBuild Error #1400" ) );
}

//------------------------------------------------------------------------------
