//
// Some exported functions
//
#if defined( __WINDOWS__ )
    #if defined( DLL_B_EXPORT )
        __declspec(dllexport) void FunctionB();
    #else
        __declspec(dllimport) void FunctionB();
    #endif
#else
    void FunctionB();
#endif
