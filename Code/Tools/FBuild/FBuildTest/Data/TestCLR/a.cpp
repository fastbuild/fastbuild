
#include "a.h"

// A managed class, which creates an unmanaged object
ref class AMaker
{
public:
    AMaker() {}
    A * MakeA()
    {
        A* a = new A;
        a->value = 15613223; // test will check for this value
        return a;
    }
};

// A function which uses CLR, exposed (in the header) via the C++ interface
A * FunctionsAsCLR_A()
{
    // use garbage collected new/clr methods
    AMaker^ aMaker = gcnew AMaker;
    return aMaker->MakeA();
}
