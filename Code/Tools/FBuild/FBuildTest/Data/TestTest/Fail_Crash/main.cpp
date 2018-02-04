//
// An simple executable to run as a test which crashes
//

#if defined( __WINDOWS__ )
    #include "windows.h" // For SetErrorMode
#endif

int main(int, char **)
{
    #if defined( __WINDOWS__ )
        // Prevent crash popups on Windows
        SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOGPFAULTERRORBOX );
    #endif

    int * i = nullptr;
    *i = 99; // nullptr deref crash

    return 0;
}
