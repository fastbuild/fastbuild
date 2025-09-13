// JSON - Helper functions to assist with JSON serialization
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "JSON.h"

// Core
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
/*static*/ void JSON::AppendEscaped( const char * string,
                                     AString & outString )
{
    const char * pos = string;
    while ( const char c = *pos++ )
    {
        // control character?
        if ( c <= 0x1F )
        {
            // escape with backslash if possible
            if ( c == '\b' )
            {
                outString += "\\b";
                continue;
            }
            if ( c == '\t' )
            {
                outString += "\\t";
                continue;
            }
            if ( c == '\n' )
            {
                outString += "\\n";
                continue;
            }
            if ( c == '\f' )
            {
                outString += "\\f";
                continue;
            }
            if ( c == '\r' )
            {
                outString += "\\r";
                continue;
            }

            // escape with codepoint
            outString.AppendFormat( "\\u%04X", c );
            continue;
        }
        else if ( c == '\"' )
        {
            // escape quotes
            outString += "\\\"";
            continue;
        }
        else if ( c == '\\' )
        {
            // escape backslashes
            outString += "\\\\";
            continue;
        }

        // char does not need escaping
        outString += c;
    }
}

//------------------------------------------------------------------------------
/*static*/ void JSON::Escape( AString & string )
{
    // Build result in a temporary buffer
    AStackString<8192> temp;
    AppendEscaped( string.Get(), temp );

    // store final result
    string = temp;
}

//------------------------------------------------------------------------------
