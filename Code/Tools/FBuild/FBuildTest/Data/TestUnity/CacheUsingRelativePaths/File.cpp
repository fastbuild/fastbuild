
#include "Subdir/Header.h"

const char* Function()
{
    // .obj file will contain filename, surrounded by these tokens
    return "FILE_MACRO_START_2(" __FILE__ ")FILE_MACRO_END_2";
}

const char* Function2()
{
    return GetFile(); // From included header
}
