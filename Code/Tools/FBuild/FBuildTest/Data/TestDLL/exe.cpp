//
// An executable using a DLL
//

// Include for the DLL
#include "a.h"

extern "C"
{
	int ExeMain(void)
	{
		return FunctionA();
	}
}
