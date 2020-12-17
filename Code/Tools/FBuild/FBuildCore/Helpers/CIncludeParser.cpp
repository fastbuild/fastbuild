// CIncludeParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CIncludeParser.h"

#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <string.h>

//------------------------------------------------------------------------------
CIncludeParser::CIncludeParser()
    : m_LastCRC1( 0 )
    , m_CRCs1( 4096, true )
    , m_LastCRC2( 0 )
    , m_CRCs2( 4096, true )
    , m_Includes( 4096, true )
#ifdef DEBUG
    , m_NonUniqueCount( 0 )
#endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CIncludeParser::~CIncludeParser() = default;

// Parse
//------------------------------------------------------------------------------
bool CIncludeParser::ParseMSCL_Output( const char * compilerOutput,
                                       size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    const char * pos = compilerOutput;
    for (;;)
    {
        const char * lineStart = pos;

        // find end of the line
        pos = strchr( pos, '\n' );
        if ( !pos )
        {
            break; // end of output
        }

        const char * lineEnd = ( lineStart < pos && pos[-1] == '\r' ) ? pos - 1 : pos;

        ASSERT( *pos == '\n' );
        ++pos; // skip \r for next line

        const char * ch = lineStart;

        // count colons in the line
        const char * colon1 = nullptr;
        for ( ; ch < lineEnd ; ++ch )
        {
            if ( *ch == ':' )
            {
                if ( colon1 == nullptr )
                {
                    colon1 = ch;
                }
                else
                {
                    break;
                }
            }
        }

        // check that we have two colons separated by at least one char
        if ( colon1 == nullptr || colon1 == lineStart ||
             *ch != ':' || (ch - colon1) < 2 )
        {
            continue; // next line
        }

        ASSERT( *ch == ':' );
        const char * colon2 = ch;

        // skip whitespace (always spaces)
        do
        {
            ++ch;
        }
        while ( *ch == ' ' );

        // must have whitespaces
        if ( ch == colon2 )
        {
            continue; // next line
        }

        const char * includeStart = ch;
        const char * includeEnd = lineEnd;

        // validates the windows path
        bool validated = ( includeStart < includeEnd );
        size_t colonCount( 0 );
        for ( ; validated && ( ch < includeEnd ); ++ch )
        {
            switch ( *ch )
            {
                // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
                case '<':
                case '>':
                case '"':
                case '|':
                case '?':
                case '*':
                {
                    validated = false;
                    break;
                }
                case ':':
                {
                    // This logic handles warnings which might otherwise appear as valid paths
                    ++colonCount;
                    if ( colonCount > 1 )
                    {
                        validated = false;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if ( validated )
        {
            const char c1 = includeStart[ 0 ];
            const bool driveLetter = ( ( ( c1 >= 'A' ) && ( c1 <= 'Z' ) ) || ( ( c1 >= 'a' ) && ( c1 <= 'z' ) ) );
            const bool validPath = driveLetter && ( includeStart[ 1 ] == ':' );
            if ( validPath )
            {
                AddInclude( includeStart, includeEnd );
            }
        }
    }

    return true;
}

// Parse
//------------------------------------------------------------------------------
bool CIncludeParser::ParseMSCL_Preprocessed( const char * compilerOutput,
                                             size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    const char * pos = compilerOutput;

    for (;;)
    {
        pos = strstr( pos, "#line 1 " );
        if ( !pos )
        {
            break;
        }

        const char * lineStart = pos;
        pos += 8;

        // search backwards for start of line
    searchForLineStart:
        // special case for first line (prevent buffer underread)
        if ( lineStart == compilerOutput )
        {
            goto foundInclude;
        }

        // skip whitespace
        --lineStart;
        if ( ( *lineStart == ' ' ) || ( *lineStart == '\t' ) )
        {
            goto searchForLineStart;
        }

        // wrapped to previous line?
        if ( *lineStart == '\n' )
        {
            goto foundInclude;
        }

        // hit some non-whitespace before the #line
        continue; // look for another #line

    foundInclude:

        // go to opening quote
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false;
        }
        pos++;

        const char * incStart = pos;

        // find end of line
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false;
        }

        const char * incEnd = pos;

        AddInclude( incStart, incEnd );
    }

    return true;
}

// ParseToNextLineStaringWithHash
//------------------------------------------------------------------------------
/*static*/ void CIncludeParser::ParseToNextLineStartingWithHash( const char * & pos )
{
    for (;;)
    {
        pos = strchr( pos, '#' );
        if ( pos )
        {
            // Safe to index -1 because # as first char is handled as a
            // special case to avoid having it in this critical loop
            const char prevC = pos[ -1 ];
            if ( ( prevC  == '\n' ) || ( prevC  == '\r' ) )
            {
                return;
            }
            ++pos;
            continue;
        }
        return;
    }
}

// Parse
//------------------------------------------------------------------------------
// TODO:C - restructure function to avoid use of gotos
PRAGMA_DISABLE_PUSH_MSVC(26051) // Function with irreducible control flow graph.
bool CIncludeParser::ParseGCC_Preprocessed( const char * compilerOutput,
                                            size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    const char * pos = compilerOutput;
    bool hasFlags = true;

    // special case for include on first line
    // (out of loop to keep loop logic simple)
    if ( pos[ 0 ] == '#' )
    {
        ++pos;
        goto possibleInclude;
    }

    for (;;)
    {
        ParseToNextLineStartingWithHash( pos );
        if ( !pos )
        {
            break;
        }
        ++pos;
    possibleInclude:
        if ( *pos == ' ' )
        {
            ++pos;
            goto foundInclude;
        }
        if ( strncmp( pos, "line ", 5 ) == 0 )
        {
            hasFlags = false;
            pos += 5;
            goto foundInclude;
        }
        continue; // some other directive we don't care about

    foundInclude:

        // skip number
        for ( ;; )
        {
            char c = * pos;
            if ( ( c >= '0' ) && ( c <= '9' ) )
            {
                pos++;
                continue;
            }
            break; // non numeric
        }

        // single space
        if ( *pos != ' ' )
        {
            continue;
        }
        pos++;

        // opening quote
        if ( *pos != '"' )
        {
            continue;
        }
        pos++;

        // ignore special case GCC "<built-in>" and "<command line>"
        if ( *pos == '<' )
        {
            continue;
        }

        const char * lineStart = pos;

        // find end of line
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false; // corrupt input
        }

        const char * lineEnd = pos;

        // ignore GCC paths
        const char lastChar( lineEnd[ -1 ] );
        if ( ( lastChar == NATIVE_SLASH ) || ( lastChar == OTHER_SLASH ) )
        {
            continue;
        }
        pos++;

        // only add an include if the preprocessor included it (indicated by the `1` flag
        // https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
        // or if it is coming from -fms-extention which doesn't have flags
        if ( strncmp( pos, " 1", 2 ) == 0 || !hasFlags )
        {
            AddInclude( lineStart, lineEnd );
        }

    }

    return true;
}
PRAGMA_DISABLE_POP_MSVC

// SwapIncludes
//------------------------------------------------------------------------------
void CIncludeParser::SwapIncludes( Array< AString > & includes )
{
    m_Includes.Swap( includes );
}

// AddInclude
//------------------------------------------------------------------------------
void CIncludeParser::AddInclude( const char * begin, const char * end )
{
    #ifdef DEBUG
        m_NonUniqueCount++;
    #endif

    // quick check
    uint32_t crc1 = xxHash::Calc32( begin, (size_t)( end - begin ) );
    if ( crc1 == m_LastCRC1 )
    {
        return;
    }
    m_LastCRC1 = crc1;
    if ( m_CRCs1.Find( crc1 ) )
    {
        return;
    }
    m_CRCs1.Append( crc1 );

    // robust check
    AStackString< 256 > include( begin, end );
    AStackString< 256 > cleanInclude;
    NodeGraph::CleanPath( include, cleanInclude );
    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Windows and OSX are case-insensitive
        AStackString<> lowerCopy( cleanInclude );
        lowerCopy.ToLower();
        uint32_t crc2 = xxHash::Calc32( lowerCopy );
    #else
        // Linux is case-sensitive
        uint32_t crc2 = xxHash::Calc32( cleanInclude );
    #endif
    if ( crc2 == m_LastCRC2 )
    {
        return;
    }
    m_LastCRC2 = crc2;
    if ( m_CRCs2.Find( crc2 ) == nullptr )
    {
        m_CRCs2.Append( crc2 );
        m_Includes.Append( cleanInclude );
    }
}

//------------------------------------------------------------------------------
