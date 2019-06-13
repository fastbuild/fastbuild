//
// dummy sandbox exe that runs a subprocess
//
#include "Core/Process/Process.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/Strings/AStackString.h"
#include <stdio.h>
#include <string.h>

void strcat_safe(
    char * dest, const char * src, const size_t maxLength )
{
#if defined( __WINDOWS__ )
    strcat_s(dest, maxLength, src);
#else
    if ( dest && src )
    {
        // find end of existing string
        char *end = static_cast<char*>( memchr( dest, '\0', maxLength ) );
        if ( end != 0 )
        {
            strncat( end, src, maxLength - (end - dest) - 1 );
        }
    }
#endif
}

int main( int argc, char** argv )
{
    const char * appName = nullptr;
    if ( argc > 0 )
    {
        appName = argv[0];
    }
    if ( argc < 2 )  // need at least the cmd param
    {
        if ( appName )
        {
            fprintf( stderr, "%s bad args!\n", appName );
        }
        else
        {
            fprintf( stderr, "bad args!\n" );
        }
        return 1;
    }

    const char * cmd = argv[1];
    
    const size_t maxChars = 4096;
    char * cmdArgs = new char[maxChars];
    cmdArgs[ 0 ] = '\0';  // terminate string
    if ( argc > 2 )  // if cmd has args
    {
        // loop over remaining args after the cmd
        for( int i = 2; i < argc; ++i )
        {
            // if arg contains spaces, then add it with quotes
            char * spacePos = strstr( argv[ i ], " " );
            if ( spacePos != nullptr )
            {
                strcat_safe( cmdArgs, "\"", maxChars );
                strcat_safe( cmdArgs, argv[ i ], maxChars );
                strcat_safe( cmdArgs, "\"", maxChars );
            }
            else  // no spaces, so add arg as-is
            {
                strcat_safe( cmdArgs, argv[ i ], maxChars );
            }
            // include a space between each arg
            strcat_safe( cmdArgs, " ", maxChars );
        }
    }

    // spawn the process
    Process p;
    ffprintf( stderr, "cmd:%s cmdArgs:%s\n", cmd, cmdArgs );
    if ( p.Spawn( cmd, cmdArgs, nullptr, nullptr ) == false )
    {
        if ( p.HasAborted() )
        {
            fprintf( stderr, "%s child '%s' process aborted!\n", appName, cmd );
            return 2;
        }

        fprintf( stderr, "%s failed to spawn child '%s' process Error: %s\n",
                appName, cmd, LAST_ERROR_STR );
        return 3;
    }

    // capture all of the stdout and stderr
    AutoPtr< char > memOut;
    AutoPtr< char > memErr;
    uint32_t memOutSize = 0;
    uint32_t memErrSize = 0;
    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

    // Get result
    const int result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return 4;
    }

    if ( memOut.Get() != nullptr )
    {
        fprintf( stdout, "%s", memOut.Get() );
    }
    if ( memErr.Get() != nullptr )
    {
        fprintf( stderr, "%s", memErr.Get() );
    }

    fprintf( stderr, "result:%d\n", result );
    return result; // test will check this
}
