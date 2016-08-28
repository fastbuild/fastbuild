
// a regular class
class A
{
public:
    A() { value = 0; }

    int value;
};

// A CLR function (accessible from C++)
A * FunctionsAsCLR_A();
