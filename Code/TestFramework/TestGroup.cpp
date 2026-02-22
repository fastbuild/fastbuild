// TestGroup.cpp - interface for a group of related tests
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestGroup.h"

// Core
#include <Core/Strings/AStackString.h>

// StaticData
//------------------------------------------------------------------------------
/*static*/ bool TestGroup::sMemoryLeakCheckEnabled = false;

//------------------------------------------------------------------------------
bool TestGroup::ShouldRun( const char * test, const Array<AString> & filters ) const
{
    // If there are no filters, all tests are run
    if ( filters.IsEmpty() )
    {
        return true;
    }

    // Pattern match filters against qualified test names
    AStackString qualifiedTestName;
    qualifiedTestName.Format( "%s.%s", GetName(), test );
    for ( const AString & filter : filters )
    {
        if ( qualifiedTestName.MatchesI( filter.Get() ) )
        {
            return true;
        }
    }

    return false;
}

// TestNoReturn
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
void TestNoReturn()
{
    #if defined( __clang__ )
    for ( ;; )
    {
    }
    #endif
}
#endif

//------------------------------------------------------------------------------
