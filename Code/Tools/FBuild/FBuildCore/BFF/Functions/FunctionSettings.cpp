// FunctionSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionSettings.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"

// Static Data
//------------------------------------------------------------------------------
/*static*/ AString FunctionSettings::s_CachePath( 64 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionSettings::FunctionSettings()
: Function( "Settings" )
{
}

// IsUnique
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSettings::IsUnique() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSettings::Commit( NodeGraph & /*nodeGraph*/, const BFFIterator & funcStartIter ) const
{
    // using a cache plugin?
    AStackString<> cachePluginDLL;
    if ( !GetString( funcStartIter, cachePluginDLL, ".CachePluginDLL" ) )
    {
        return false;
    }
    FBuild::Get().SetCachePluginDLL( cachePluginDLL );
    if ( !cachePluginDLL.IsEmpty() )
    {
        FLOG_INFO( "CachePluginDLL: '%s'", cachePluginDLL.Get() );
    }

    // try to get the cache path from the config
    const BFFVariable * cachePathVar;
    if ( !GetString( funcStartIter, cachePathVar, ".CachePath" ) )
    {
        return false;
    }
    if ( cachePathVar )
    {
        s_CachePath = cachePathVar->GetString();

        // override environment default only if not empty
        if ( s_CachePath.IsEmpty() == false )
        {
            FBuild::Get().SetCachePath( s_CachePath );
        }
    }

    // "Workers"
    Array< AString > workerList;
    if ( !GetStrings( funcStartIter, workerList, ".Workers" ) )
    {
        return false;
    }
    if ( !workerList.IsEmpty() )
    {
        FBuild::Get().SetWorkerList( workerList );
    }

    // "Environment"
    Array< AString > environment;
    if ( !GetStrings( funcStartIter, environment, ".Environment" ) )
    {
        return false;
    }
    if ( !environment.IsEmpty() )
    {
        ProcessEnvironment( environment );
    }

    return true;
}

// ProcessEnvironment
//------------------------------------------------------------------------------
void FunctionSettings::ProcessEnvironment( const Array< AString > & envStrings ) const
{
    // the environment string is used in windows as a double-null terminated string
    // so convert our array to a single buffer

    // work out space required
    uint32_t size = 0;
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        size += envStrings[ i ].GetLength() + 1; // string len inc null
    }

    // allocate space
    AutoPtr< char > envString( (char *)ALLOC( size + 1 ) ); // +1 for extra double-null

    // while iterating, extract the LIB environment variable (if there is one)
    AStackString<> libEnvVar;

    // copy strings end to end
    char * dst = envString.Get();
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        if ( envStrings[ i ].BeginsWith( "LIB=" ) )
        {
            libEnvVar.Assign( envStrings[ i ].Get() + 4, envStrings[ i ].GetEnd() );
        }

        const uint32_t thisStringLen = envStrings[ i ].GetLength();
        AString::Copy( envStrings[ i ].Get(), dst, thisStringLen );
        dst += ( thisStringLen + 1 );
    }

    // final double-null
    *dst = '\000';

    FBuild::Get().SetEnvironmentString( envString.Get(), size, libEnvVar );
}

//------------------------------------------------------------------------------
