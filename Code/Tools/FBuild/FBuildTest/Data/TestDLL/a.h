//
// Some exported functions
//
#if defined( __WINDOWS__ )
    __declspec(dllexport) int FunctionA();
#else
    int FunctionA();
#endif
