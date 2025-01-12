// TestMemInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Mem/MemInfo.h"

// TestMemInfo
//------------------------------------------------------------------------------
class TestMemInfo : public TestGroup
{
private:
    DECLARE_TESTS

    void GetInfo() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMemInfo )
    REGISTER_TEST( GetInfo )
REGISTER_TESTS_END

// GetInfo
//------------------------------------------------------------------------------
void TestMemInfo::GetInfo() const
{
    SystemMemInfo info;
    MemInfo::GetSystemInfo( info );

    // Assume we should have at least 1 GiB physical memory
    TEST_ASSERT( info.mTotalPhysMiB > 1024 );

    // Assume at least some physical memory is available when test is run
    TEST_ASSERT( info.mAvailPhysMiB > 1 );

    // Available can't be more than physical and should never be equal
    // as that would assume the OS and all running processes use zero
    // memory
    TEST_ASSERT( info.mAvailPhysMiB < info.mTotalPhysMiB );
}

//------------------------------------------------------------------------------
