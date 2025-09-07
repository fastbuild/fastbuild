// TestMemInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Mem/MemInfo.h"

// TestMemInfo
//------------------------------------------------------------------------------
class TestMemInfo : public TestGroup
{
private:
    DECLARE_TESTS

    void GetSystemInfo() const;
    void GetProcessInfo() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMemInfo )
    REGISTER_TEST( GetSystemInfo )
    REGISTER_TEST( GetProcessInfo )
REGISTER_TESTS_END

//------------------------------------------------------------------------------
void TestMemInfo::GetSystemInfo() const
{
    SystemMemInfo info;
    MemInfo::GetSystemInfo( info );

    // Assume we should have at least 1 GiB physical memory
    TEST_ASSERT( info.m_TotalPhysMiB > 1024 );

    // Assume at least some physical memory is available when test is run
    TEST_ASSERT( info.m_AvailPhysMiB > 1 );

    // Available can't be more than physical and should never be equal
    // as that would assume the OS and all running processes use zero
    // memory
    TEST_ASSERT( info.m_AvailPhysMiB < info.m_TotalPhysMiB );
}

//------------------------------------------------------------------------------
void TestMemInfo::GetProcessInfo() const
{
    static const size_t sizesInBytes[] = { 1 * 1024 * 1024,
                                           16 * 1024 * 1024,
                                           128 * 1024 * 1024 };

    // Allocate some memory to ensure at least that much is reported as used
    UniquePtr<char, FreeDeletor> mem;
    for ( const size_t sizeInBytes : sizesInBytes )
    {
        mem.Replace( static_cast<char *>( ALLOC( sizeInBytes ) ) );
        ASSERT( MemInfo::GetProcessInfo() >= MemInfo::ConvertBytesToMiB( sizeInBytes ) );
    }
}

//------------------------------------------------------------------------------
