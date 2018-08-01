//
// dummy sandbox exe that runs a subprocess
//
#include "Core/Process/Process.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"
#include <stdio.h>

int main(int argc, char** argv)
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
            printf( "%s bad args!\n", appName );
        }
        else
        {
            printf( "bad args!\n" );
        }
        return 1;
    }

    const char * cmd = argv[1];
    
    char * cmdArgs = new char[1024];
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
                strcat( cmdArgs, "\"");
                strcat( cmdArgs, argv[ i ] );
                strcat( cmdArgs, "\"");
            }
            else  // no spaces, so add arg as-is
            {
                strcat( cmdArgs, argv[ i ] );
            }
            // include a space between each arg
            strcat( cmdArgs, " ");
        }
    }

    // spawn the process
    Process p;
    if ( p.Spawn( cmd, cmdArgs, nullptr, nullptr ) == false )
    {
        if ( p.HasAborted() )
        {
            printf( "%s child '%s' process aborted!\n", appName, cmd );
            return 2;
        }

        printf( "%s failed to spawn child '%s' process (error 0x%x)\n",
                appName, cmd, Env::GetLastErr() );
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
        printf( "%s", memOut.Get() );
    }
    if ( memErr.Get() != nullptr )
    {
        printf( "%s", memErr.Get() );
    }

    return result; // test will check this
}
