// TestFBuildOptions.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuildOptions.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AStackString.h"

// TestFBuildOptions
//------------------------------------------------------------------------------
TEST_GROUP( TestFBuildOptions, FBuildTest )
{
public:
    FBuildOptions::OptionsResult Parse( FBuildOptions & options,
                                        const char * a1 = nullptr,
                                        const char * a2 = nullptr,
                                        const char * a3 = nullptr ) const;

    void operator=( TestFBuildOptions & ) = delete;
};

//------------------------------------------------------------------------------
TEST_CASE( TestFBuildOptions, DTLTOFile )
{
    FBuildOptions options;
    TEST_ASSERT( Parse( options, "-dtlto", "dist-file.json" ) == FBuildOptions::OPTIONS_OK );
    TEST_ASSERT( options.m_DTLTOFile == "dist-file.json" );
    TEST_ASSERT( options.GetArgs().Find( "dist-file.json" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestFBuildOptions, DTLTOFileMissingArgErrors )
{
    FBuildOptions options;
    TEST_ASSERT( Parse( options, "-dtlto" ) == FBuildOptions::OPTIONS_ERROR ); // no <path> follows
    TEST_ASSERT( options.m_DTLTOFile.IsEmpty() );
}

// Parse
//------------------------------------------------------------------------------
FBuildOptions::OptionsResult TestFBuildOptions::Parse( FBuildOptions & options,
                                                       const char * a1,
                                                       const char * a2,
                                                       const char * a3 ) const
{
    StackArray<char *> argv;
    argv.Append( const_cast<char *>( "FBuild.exe" ) ); // argv[0] is the program name
    if ( a1 )
    {
        argv.Append( const_cast<char *>( a1 ) );
    }
    if ( a2 )
    {
        argv.Append( const_cast<char *>( a2 ) );
    }
    if ( a3 )
    {
        argv.Append( const_cast<char *>( a3 ) );
    }
    return options.ProcessCommandLine( static_cast<int32_t>( argv.GetSize() ), argv.Begin() );
}

//------------------------------------------------------------------------------
