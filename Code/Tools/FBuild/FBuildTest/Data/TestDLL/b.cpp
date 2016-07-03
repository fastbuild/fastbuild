//
// Some exported functions
//
#include "b.h"

#if defined( __WINDOWS__ )
    bool __stdcall DllMain( void *, unsigned int, void * )
    {
        return true;
    }
#endif

void FunctionB()
{
}
