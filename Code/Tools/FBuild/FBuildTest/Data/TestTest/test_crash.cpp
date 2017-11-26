//
// An test that crashes
//

int main(int , char **)
{
    volatile int * volatile p = 0;
    *p = 42;
    return 0;
}
