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

//------------------------------------------------------------------------------
TEST_GROUP( TestCopy, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToFile )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString dst( "../tmp/Test/Copy/copy.bff.copy" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToFile" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/filetofile.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dst );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 1, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToFile_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/filetofile.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToFile" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToFile_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/filetofile.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToFile" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToDir )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString dst( "../tmp/Test/Copy/copy.bff" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToDir" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/filetodir.fdb" ) );

    EnsureFileExists( dst );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 1, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToDir_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/filetodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_FileToDir_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/filetodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestCopyFileToDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 3, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_MultiFileToDir )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString dst1( "../tmp/Test/Copy/a.txt" );
    const AStackString dst2( "../tmp/Test/Copy/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst1 );
    EnsureFileDoesNotExist( dst2 );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestMultiCopyToDir" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/multifiletodir.fdb" ) );

    EnsureFileExists( dst1 );
    EnsureFileExists( dst2 );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 2, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_MultiFileToDir_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/multifiletodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestMultiCopyToDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_MultiFileToDir_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/multifiletodir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestMultiCopyToDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_SourceBasePath )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString dst1( "../tmp/Test/Copy/SourceBasePath/TestCopy/a.txt" );
    const AStackString dst2( "../tmp/Test/Copy/SourceBasePath/TestCopy/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst1 );
    EnsureFileDoesNotExist( dst2 );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestSourceBasePath" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/SourceBasePath/sourcebasepath.fdb" ) );

    EnsureFileExists( dst1 );
    EnsureFileExists( dst2 );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 2, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, TestCopyFunction_SourceBasePath_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/SourceBasePath/sourcebasepath.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TestSourceBasePath" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, ChainedCopy )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString dst( "../tmp/Test/Copy/ChainedCopy/copy.copy3" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dst );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ChainedCopy3" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/ChainedCopy/chainedcopy.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dst );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 3, 3, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, ChainedCopy_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/ChainedCopy/chainedcopy.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ChainedCopy3" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::FILE_NODE );
    CheckStatsNode( 3, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 5, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, CopyDir )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    AStackString dstA( "../tmp/Test/Copy/CopyDir/a.txt" );
    AStackString dstB( "../tmp/Test/Copy/CopyDir/b.txt" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( dstA );
    EnsureFileDoesNotExist( dstB );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "CopyDir" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/CopyDir/copydir.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( dstA );
    EnsureFileExists( dstB );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 2, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 6, 6 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, CopyDir_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/CopyDir/copydir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "CopyDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_DIR_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 6, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, CopyDir_NoRebuild_BFFChange )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/CopyDir/copydir.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "CopyDir" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE );
    CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    CheckStatsNode( 1, 0, Node::COPY_DIR_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 6, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, CopyDirDeleteSrc )
{
    // We'll operate on copies of the source files so we can delete on of the src files
    AStackString srcA( "../tmp/Test/Copy/CopyDirDeleteSrc/Src/a.txt" );
    AStackString srcB( "../tmp/Test/Copy/CopyDirDeleteSrc/Src/b.txt" );
    AStackString dstA( "../tmp/Test/Copy/CopyDirDeleteSrc/Dst/a.txt" );
    AStackString dstB( "../tmp/Test/Copy/CopyDirDeleteSrc/Dst/b.txt" );

    // Do the normal build, which copies the files
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Set up the source
        TEST_ASSERT( FileIO::EnsurePathExists( AStackString( "../tmp/Test/Copy/CopyDirDeleteSrc/Src/" ) ) );
        TEST_ASSERT( FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestCopy/a.txt", srcA.Get() ) );
        TEST_ASSERT( FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestCopy/b.txt", srcB.Get() ) );
        TEST_ASSERT( FileIO::SetReadOnly( srcA.Get(), false ) ); // Clear read only so it's not persisted by copy
        TEST_ASSERT( FileIO::SetReadOnly( srcB.Get(), false ) ); // Clear read only so it's not persisted by copy

        // clean up anything left over from previous runs
        EnsureFileDoesNotExist( dstA );
        EnsureFileDoesNotExist( dstB );

        // build (via alias)
        TEST_ASSERT( fBuild.Build( "CopyDirDeleteSrc" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Copy/CopyDir/copydirdeletesrc.fdb" ) );

        // make sure all output is where it is expected
        EnsureFileExists( dstA );
        EnsureFileExists( dstB );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 2, Node::FILE_NODE );
        CheckStatsNode( 2, 2, Node::COPY_FILE_NODE );
        CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
        CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
        CheckStatsTotal( 6, 6 );
    }

    // Delete one of the source files
    EnsureFileDoesNotExist( srcB );

    // Build again to make sure it doesn't fail
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Copy/CopyDir/copydirdeletesrc.fdb" ) );

        // build (via alias)
        TEST_ASSERT( fBuild.Build( "CopyDirDeleteSrc" ) );

        // make sure all output is where it is expected
        EnsureFileExists( dstA );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 1, Node::FILE_NODE );
        CheckStatsNode( 1, 0, Node::COPY_FILE_NODE );
        CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
        CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
        CheckStatsTotal( 4, 3 );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, CopyEmpty )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/copy.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "CopyEmpty" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, MissingTrailingSlash )
{
    // Ensure a copy of multiple files to destination that is not a path
    // is reported as an explicit error
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/MissingTrailingSlash/fbuild.bff";
    FBuildForTest fBuild( options );

    // Parsing should fail
    TEST_ASSERT( fBuild.Initialize() == false );

    // Ensure the explicit error for this case is reported (not a generic one about
    // the target already being defined)
    TEST_ASSERT( GetRecordedOutput().Find( "FASTBuild Error #1400" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, ObjectListChaining )
{
    //  - Ensure that a CopyDir consuming the output of an ObjectList
    //    is managed correctly
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCopy/ObjectListChaining/fbuild.bff";
    const char * dbFile = "../tmp/Test/Copy/ObjectListChaining/fbuild.fdb";

    AString depGraphText1( 8 * 1024 );
    AString depGraphText2( 8 * 1024 );

    // Build
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "CopyDir" ) );

        // Save DB for reloading below
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 2, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
        CheckStatsNode( 2, 2, Node::COPY_FILE_NODE );

        fBuild.SerializeDepGraphToText( "CopyDir", depGraphText1 );
    }

    // Check no-rebuild
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "CopyDir" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1, 0, Node::COPY_DIR_NODE );
        CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );
    }

    // Check no-rebuild DB migration
    {
        FBuildOptions options2( options );
        options2.m_ForceDBMigration_Debug = true;

        FBuildForTest fBuild( options2 );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "CopyDir" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1, 0, Node::COPY_DIR_NODE );
        CheckStatsNode( 2, 0, Node::COPY_FILE_NODE );

        fBuild.SerializeDepGraphToText( "CopyDir", depGraphText2 );
    }

    // Check node graph is the same after migration
    TEST_ASSERT( depGraphText1 == depGraphText2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCopy, ObjectListChaining2 )
{
    //  - Ensure that a CopyDir consuming the output of an ObjectList
    //    is managed correctly
    //
    //  - This test explicitly checks that CopyFile node from a previous build
    //    are not migrated if the ObjectList it depends on is not migrated.
    //    When it was migrated in this situation, it causes a bug because the CopyFile
    //    node created a FileNode for the source, but that source was supposed to be an
    //    ObjectNode. This caused the build to fail and the DB to be unrecoverably corrupt.
    const char * configFileOriginal = "Tools/FBuild/FBuildTest/Data/TestCopy/ObjectListChaining2/fbuild.bff";
    const char * configFileCopy = "../tmp/Test/Copy/ObjectListChaining2/fbuild.bff";

    FileIO::EnsurePathExistsForFile( AStackString( configFileCopy ) );
    VERIFY( FileIO::FileCopy( configFileOriginal, configFileCopy ) );
    VERIFY( FileIO::SetReadOnly( configFileCopy, false ) );

    FBuildTestOptions options;
    options.m_ConfigFile = configFileCopy;
    const char * dbFile = "../tmp/Test/Copy/ObjectListChaining2/fbuild.fdb";

    AString depGraphText1( 8 * 1024 );
    AString depGraphText2( 8 * 1024 );

    // Build
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "CopyDir" ) );

        // Save DB for reloading below
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 1, 1, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
        CheckStatsNode( 1, 1, Node::COPY_FILE_NODE );

        fBuild.SerializeDepGraphToText( "CopyDir", depGraphText1 );
    }

    // Modify the config file in a way that dirties the ObjectList only
    AString buffer;
    {
        FileStream f;
        VERIFY( f.Open( configFileCopy ) );
        buffer.SetLength( (uint32_t)f.GetFileSize() );
        VERIFY( f.ReadBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() );
    }
    VERIFY( buffer.Replace( "-DEXTRA_A", "-DEXTRA_B" ) == 1 );
    {
        FileStream f;
        VERIFY( f.Open( configFileCopy, FileStream::WRITE_ONLY ) );
        VERIFY( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() );
    }

    // Check rebuild
    {
        options.m_ForceDBMigration_Debug = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "CopyDir" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 1, 1, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1, 1, Node::COPY_DIR_NODE );
        CheckStatsNode( 1, 1, Node::COPY_FILE_NODE );

        fBuild.SerializeDepGraphToText( "CopyDir", depGraphText2 );
    }

    // Check node graph is the same after migration
    TEST_ASSERT( depGraphText1 == depGraphText2 );
}

//------------------------------------------------------------------------------
