
// Avoid include of stdio.h to improve compile time
extern "C"
{
    void printf( const char * format, ... );
}

int main( int argc, char * argv[] )
{
    // Echo the args we received so test can test end-to-end
    // handling of args
    // Exclude the executable (arg 0)
    printf( "%i\n", argc - 1 );
    for ( int i = 1; i < argc; ++i )
    {
        printf( "%s\n", argv[ i ] );
    }
    return 0;
}
