//
// A simple compiler that concatenates its inputs to output
//
#include <stdio.h>

static bool AppendFile( FILE * dst, const char * fileName )
{
    FILE * f = fopen( fileName, "rb" );
    if ( f == nullptr )
    {
        fprintf( stderr, "Failed to open '%s'\n", fileName );
        return false;
    }

    char buffer[ 1024 ];
    for ( ;; )
    {
        const size_t numRead = fread( buffer, 1, sizeof( buffer ), f );
        if ( numRead == 0 )
        {
            break;
        }
        if ( fwrite( buffer, 1, numRead, dst ) != numRead )
        {
            fclose( f );
            return false;
        }
    }
    fclose( f );
    return true;
}

int main( int argc, char ** argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr, "Bad Args!\n" );
        return 1;
    }

    FILE * out = fopen( argv[ 2 ], "wb" );
    if ( out == nullptr )
    {
        fprintf( stderr, "Failed to open '%s'\n", argv[ 2 ] );
        return 1;
    }

    const bool ok = AppendFile( out, argv[ 1 ] ) &&
                    AppendFile( out, argv[ 3 ] );
    fclose( out );
    return ok ? 0 : 1;
}
