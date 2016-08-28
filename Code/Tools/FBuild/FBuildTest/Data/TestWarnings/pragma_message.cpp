
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define PRAGMA_OPTIMIZE_OFF __pragma(optimize("", off)) __pragma(message(__FILE__ "(" STRING(__LINE__) "): warning : Optimization force disabled"))

PRAGMA_OPTIMIZE_OFF

void Function()
{
}
