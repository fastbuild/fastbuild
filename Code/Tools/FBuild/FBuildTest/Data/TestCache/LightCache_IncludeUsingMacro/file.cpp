//
// LightCache does not support includes which use macros as follows
// but should detect this case and safely detect this case
//
//------------------------------------------------------------------------------
#define PATH_AS_MACRO <string.h>
#include PATH_AS_MACRO
