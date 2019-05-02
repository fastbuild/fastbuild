namespace
{
    int AnonFunction()
    {
        return 7;
    }
}

class A
{
public:
    static int Function()
    {
        return AnonFunction();
    }
};

