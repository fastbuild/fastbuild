
void Function()
{
    // this invalid code generates a compile error with % in it
    // which was causing a crash due to unsfae string formatting
    int %s%s%s%s
}
