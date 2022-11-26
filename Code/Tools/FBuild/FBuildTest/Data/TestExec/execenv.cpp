//
// An simple executable to test the environment variable FB_ENV_TEST_VALUE is set
//
#if defined( __WINDOWS__ )
    #include <Windows.h>
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    #include <stdlib.h>
#else
    #error Unknown platform
#endif

#include <string.h>

int main(int, char *[], char *[])
{
    #if defined( __WINDOWS__ )
        char envVar[15];
        DWORD ret = GetEnvironmentVariable( "FB_ENV_TEST_VALUE", envVar, 15 );

        // variable does not exist or is longer than expected
        if ( ret == 0 || ret > 9 )
        {
            return 1;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        const char * envVar = getenv( "FB_ENV_TEST_VALUE" );
        if ( !envVar )
        {
            return 1;
        }
    #else
        #error Unknown platform
    #endif
    
    if ( strcmp( envVar, "FASTbuild" ) == 0 )
    {
        return 0;
    }
    return 1;
}
