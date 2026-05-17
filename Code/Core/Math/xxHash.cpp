// xxHash.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "xxHash.h"

// Core
#include "Core/Containers/Array.h"

//------------------------------------------------------------------------------
/*static*/ uint64_t xxHash3::Calc64( const Array<AString> & strings )
{
    xxHash3Accumulator acc;
    for ( const AString & string : strings )
    {
        acc.AddData( string );
    }
    return acc.Finalize64();
}

//------------------------------------------------------------------------------
