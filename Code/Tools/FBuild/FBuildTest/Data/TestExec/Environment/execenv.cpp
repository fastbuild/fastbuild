//
// An simple executable to test the environment variable FB_ENV_TEST_VALUE is set
//
extern "C"
{
    // Avoid include files to speed up test compilation
    #if defined( __WINDOWS__ )
        unsigned int GetEnvironmentVariableA( const char * name, char * buffer, unsigned int size );
    #else
        char * getenv( const char * name );
    #endif
}

int main(int, char *[], char *[])
{
    // Check if env variable exists
    #if defined( __WINDOWS__ )
        const bool ok = ( GetEnvironmentVariableA( "FB_ENV_TEST_VALUE", nullptr, 0 ) != 0 );
    #else
        const bool ok = ( getenv( "FB_ENV_TEST_VALUE" ) != nullptr );
    #endif
    return ok ? 0 : 1; // Zero for success
}
