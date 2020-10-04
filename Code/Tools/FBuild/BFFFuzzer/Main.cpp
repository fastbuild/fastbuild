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

class FuzzerEnvironment
{
public:
    FuzzerEnvironment()
        : m_fbuild( GetOptions() )
    {
        Tracing::AddCallbackOutput( AlwaysFalse );
    }
    ~FuzzerEnvironment()
    {
        Tracing::RemoveCallbackOutput( AlwaysFalse );
    }

private:
    static FBuildOptions GetOptions()
    {
        FBuildOptions options;
        options.m_ShowPrintStatements = false; // disable output from Print function TODO:C Why is this needed?
        options.m_ShowErrors = false; // disable error messages
        return options;
    }

    FBuild m_fbuild;
};

extern "C" int LLVMFuzzerTestOneInput( const uint8_t * data, size_t size )
{
    static FuzzerEnvironment env;

    // Because BFFParser expects null-terminated input, we have to make a copy of the data and append null.
    UniquePtr< char > str( (char *)ALLOC( size + 1 ) );
    memcpy( str.Get(), data, size );
    str.Get()[ size ] = 0;

    NodeGraph ng;
    BFFParser p( ng );
    p.ParseFromString( "fuzz.bff", str.Get() );

    return 0; // Non-zero return values are reserved for future use.
}
