//
// An test that fails to return
//

int main(int , char **)
{
    for (;;)
    {
        // busy wait spin to avoid pulling dependencies for sleep/yield
    }
    #if defined( __GNUC__ )
        // GCC 4.8 warns about function not returning if value is return is omitted.
        // MSVC warns about unreachable code is the return is present.
        return 0;
    #endif
}
