
// a managed type
ref class B
{
public:
    int i;
};

void FunctionsAsCLR_B()
{
    // use garbage collected new
    B^ b = gcnew B;
    (void)b;
}
