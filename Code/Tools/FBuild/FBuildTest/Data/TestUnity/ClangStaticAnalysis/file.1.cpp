
int DivideByZeroFunction( int value )
{
    return ( value / 0 ); // Generates warning
}

void OutOfScopePointerToStack()
{
    static int * x;
    int y;
    x = &y; // Generates warning
}
