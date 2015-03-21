

#include "a.h"

A* CLRCaller()
{
	return FunctionsAsCLR_A();
}

extern "C"
{
	int ExeMain(void)
	{
		// call code in the CLR library
		A * a = CLRCaller();

		return a->value; // test will check this
	}
}
