//
// Some exported functions
//
#if defined( __WINDOWS__ )
    __declspec(dllexport) void FunctionB();
#else
    void FunctionB();
#endif
