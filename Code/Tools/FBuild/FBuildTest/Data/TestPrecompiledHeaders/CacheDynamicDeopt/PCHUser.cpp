// This cpp file uses the precompiled header, and (via /dynamicdeopt) will
// produce an additional PCHUser.alt.obj file alongside PCHUser.obj.

#include <PrecompiledHeader.h>

int main( int, char *[] )
{
    return PchHelper() - 42;
}
