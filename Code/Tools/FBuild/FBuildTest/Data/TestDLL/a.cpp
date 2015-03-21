//
// Some exported functions
//
#include "PrecompiledHeader.h"
#include "a.h"


bool __stdcall DllMain( void * hinstDLL, unsigned int fdwReason, void * lpvReserved )
{
	return true;
}

int FunctionA()
{
	return 12345678; // Checked for in unit test
}
