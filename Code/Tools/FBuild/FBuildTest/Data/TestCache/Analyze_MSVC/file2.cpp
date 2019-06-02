//
// MSVC can detect some usage errors for code annotated with SAL
//

//
// This header will conditionally enable SAL depending on the _PREFAST_
// define
//
#include <string>

void Func()
{
    std::string s( nullptr );
}

