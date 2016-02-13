//
// Some exported functions
//
#include <PrecompiledHeader.h>
#include "a.h"

#if defined( __WINDOWS__ )
    bool __stdcall DllMain( void * hinstDLL, unsigned int fdwReason, void * lpvReserved )
    {
        return true;
    }
#endif

int FunctionA()
{
	return 99; // Checked for in unit test
}
