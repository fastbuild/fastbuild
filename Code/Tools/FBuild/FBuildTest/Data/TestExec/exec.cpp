//
// A simple executable to run
//
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[], char *[])
{
    // Touch each file listed
    for (int i = 1; i < argc; ++i)
    {
        const char * arg = argv[i];

        // Make a new filename based on the input
        char filename[ 1024 ] = { 0 };
        strcat( filename, arg );
        strcat( filename, ".out" );

        // Touch the file
        FILE * file = fopen( filename, "wb" );
        fwrite( "T", 1, 1, file );
        fclose( file );

        // Generate some output
        // on STDOUT based on the arguments too
        printf( "Touched: %s\n", filename );
    }

    return argc - 1;
}

