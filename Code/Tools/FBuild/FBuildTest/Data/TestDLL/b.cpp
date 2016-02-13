//
// Some exported functions
//
#include "b.h"

#if defined( __WINDOWS__ )
    bool __stdcall DllMain( void * hinstDLL, unsigned int fdwReason, void * lpvReserved )
    {
        return true;
    }
#endif

void FunctionB()
{
}
