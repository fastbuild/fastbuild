// ErrorFormat - Format errors in a consistent way
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"

// Forward Declarations
//------------------------------------------------------------------------------

// Defines
//------------------------------------------------------------------------------
#define LAST_ERROR_STR      ErrorFormat( Env::GetLastErr() ).GetString()
#define ERROR_STR( error )  ErrorFormat( (uint32_t)error ).GetString()

// ErrorFormat
//------------------------------------------------------------------------------
class ErrorFormat
{
public:
    ErrorFormat();
    explicit ErrorFormat( uint32_t error );
    ~ErrorFormat();

    const char * GetString() const { return m_Buffer.Get(); }

private:
    void Format( uint32_t error );

    AStackString<32> m_Buffer;
};

//------------------------------------------------------------------------------
