
inline const char* GetFile()
{
    // .obj file will contain filename, surrounded by these tokens
    return "FILE_MACRO_START_1(" __FILE__ ")FILE_MACRO_END_1";
}
