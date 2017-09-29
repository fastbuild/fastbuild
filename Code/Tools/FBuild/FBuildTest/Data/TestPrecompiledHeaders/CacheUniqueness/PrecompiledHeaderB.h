#ifndef PCH_INCLUDED
#define PCH_INCLUDED

// A define which is not used within the PCH, but should still uniquify the output
// because files compiled using the PCH might rely on the define
#define SPECIAL_DEFINE 2

#endif // PCH_INCLUDED
