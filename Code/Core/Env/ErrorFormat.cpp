// ErrorFormat - Format errors in a consistent way
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ErrorFormat.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ErrorFormat::ErrorFormat()
{
    Format( Env::GetLastErr() );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
ErrorFormat::ErrorFormat( uint32_t error )
{
    Format( error );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ErrorFormat::~ErrorFormat() = default;

// Format
//------------------------------------------------------------------------------
void ErrorFormat::Format( uint32_t error )
{
    if ( error <= 0xFF )
    {
        // 255 (0xFF)
        m_Buffer.Format( "%u (0x%02x)", error, error );
    }
    else if ( error <= 0xFFFF )
    {
        // 65535 (0xFFFF)
        m_Buffer.Format( "%u (0x%04x)", error, error );
    }
    else
    {
        // 4294967295 (0xFFFFFFFF)
        m_Buffer.Format( "%u (0x%08x)", error, error );
    }
}

//------------------------------------------------------------------------------
