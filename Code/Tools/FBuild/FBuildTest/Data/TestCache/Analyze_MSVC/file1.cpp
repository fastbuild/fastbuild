//
// MSVC Static Analysis can detect errors such as buffer overruns
// with no annotation
//
#define BUFFER_SIZE 32

void Func()
{
    char buffer[ BUFFER_SIZE ];

    // Out of bounds array access
    buffer[ BUFFER_SIZE ] = '\0';
}
