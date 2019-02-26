#include "a.h"
#include "b.h"

// These special tags cause a.h and b.h to be listed as dependencies by the DependencyProcessor.exe.
//:a.h
//:b.h

int main(int, const char**)
{
	return 0;
}
