
// a managed type
ref class C
{
public:
    int i;
};

void FunctionsAsCLR_C()
{
    // use garbage collected new
    C^ c = gcnew C;
    (void)c;
}
