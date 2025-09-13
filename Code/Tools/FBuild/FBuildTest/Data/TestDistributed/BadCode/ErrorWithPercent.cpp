void Function()
{
    // this invalid code generates a compile error with % in it
    // which was causing a crash due to unsfae string formatting
    // clang-format off
    int %s%s%s%s
    // clang-format on
}
