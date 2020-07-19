const char * Function()
{
    // .obj file will contain filename, surrounded by these tokens
    return "FILE_MACRO_START(" __FILE__ ")FILE_MACRO_END";
}
