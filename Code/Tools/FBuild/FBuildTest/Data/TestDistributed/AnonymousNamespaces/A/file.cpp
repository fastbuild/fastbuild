namespace
{
    int AnonFunction()
    {
        return 7;
    }
}

class A
{
    static int Function()
    {
        return AnonFunction();
    }
};

