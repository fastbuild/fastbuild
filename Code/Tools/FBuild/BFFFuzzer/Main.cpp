// BFFFuzzer
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/UniquePtr.h"
#include "Core/Mem/Mem.h"
#include "Core/Tracing/Tracing.h"

#include <memory.h> // for memcpy

bool AlwaysFalse( const char * )
{
    return false;
}

class FBuildForFuzzing : public FBuild
{
public:
    void ResetState()
    {
        FREE( m_EnvironmentString );
        m_EnvironmentString = nullptr;
        m_EnvironmentStringSize = 0;
        m_LibEnvVar.Clear();

        m_ImportedEnvironmentVars.Clear();
        m_FileExistsInfo = BFFFileExists();
        m_UserFunctions.Clear();
    }
};

class FuzzerEnvironment
{
public:
    FuzzerEnvironment()
    {
        // Disable all output from FASTBuild as it would mess up messages displayed by libFuzzer.
        Tracing::AddCallbackOutput( AlwaysFalse );
    }
    ~FuzzerEnvironment()
    {
        Tracing::RemoveCallbackOutput( AlwaysFalse );
    }

    FBuildForFuzzing fbuild;
};

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    static FuzzerEnvironment env;

    // Because BFFParser expects null-terminated input, we have to make a copy of the data and append null.
    UniquePtr< char > str( (char *)ALLOC( size + 1 ) );
    memcpy( str.Get(), data, size );
    str.Get()[ size ] = 0;

    // We don't want to recreate FBuild each time as this is expensive operation.
    // But we need to reset some of its state so it wont leak between different runs.
    env.fbuild.ResetState();

    NodeGraph ng( 8 ); // Use smaller hash table to speed up fuzzing.
    BFFParser p( ng );
    p.ParseFromString( "fuzz.bff", str.Get() );

    return 0; // Non-zero return values are reserved for future use.
}
