namespace
{
    int AnonFunction()
    {
        return 7;
    }
}

class B
{
    static int Function()
    {
        return AnonFunction();
    }
};

