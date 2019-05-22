namespace
{
    int AnonFunction()
    {
        return 7;
    }
}

class B
{
public:
    static int Function()
    {
        return AnonFunction();
    }
};

