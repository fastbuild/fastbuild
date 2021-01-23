// JSON - Helper functions to assist with JSON serialization
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "JSON.h"

// Core
#include "Core/Strings/AStackString.h"

// NEscape
//------------------------------------------------------------------------------
/*static*/ void JSON::Escape( AString & string )
{
    // Build result in a temporary buffer
    AStackString< 8192 > temp;

    const char * const end = string.GetEnd();
    for ( const char * pos = string.Get(); pos != end; ++pos )
    {
        const char c = *pos;

        // congrol character?
        if ( c <= 0x1F )
        {
            // escape with backslash if possible
            if ( c == '\b' ) { temp += "\\b"; continue; }
            if ( c == '\t' ) { temp += "\\t"; continue; }
            if ( c == '\n' ) { temp += "\\n"; continue; }
            if ( c == '\f' ) { temp += "\\f"; continue; }
            if ( c == '\r' ) { temp += "\\r"; continue; }

            // escape with codepoint
            temp.AppendFormat( "\\u%04X", c );
            continue;
        }
        else if ( c == '\"' )
        {
            // escape quotes
            temp += "\\\"";
            continue;
        }
        else if ( c == '\\' )
        {
            // escape backslashes
            temp += "\\\\";
            continue;
        }

        // char does not need escaping
        temp += c;
    }

    // store final result
    string = temp;
}

//------------------------------------------------------------------------------
