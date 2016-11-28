//
// Some exported functions
//
#if defined( __WINDOWS__ )
    #if defined( DLL_A_EXPORT )
        __declspec(dllexport) int FunctionA();
    #else
        __declspec(dllimport) int FunctionA();
    #endif
#else
    int FunctionA();
#endif
