// JSON - Helper functions to assist with JSON serialization
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// JSON
//------------------------------------------------------------------------------
class JSON
{
public:
    static void AppendEscaped( const char * string, AString & outEscapedString );
    static void Escape( AString & string );
};

//------------------------------------------------------------------------------
