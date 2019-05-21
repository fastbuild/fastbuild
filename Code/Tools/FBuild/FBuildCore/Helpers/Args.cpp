// Args
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Args.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Profile/Profile.h"
#include "Core/Strings/AString.h"

// system
#if defined( __OSX__ )
    #include <limits.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
Args::Args()
    : m_Args()
    , m_ResponseFileArgs()
    , m_DelimiterIndices( 64, true )
    #if defined( ASSERTS_ENABLED )
        , m_Finalized( false )
    #endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Args::~Args() = default;

// operator += (char *)
//------------------------------------------------------------------------------
void Args::operator += ( const char * argPart )
{
    ASSERT( !m_Finalized );
    m_Args += argPart;
}

// operator += (AString&)
//------------------------------------------------------------------------------
void Args::operator += ( const AString & argPart )
{
    ASSERT( !m_Finalized );
    m_Args += argPart;
}

// Append
//------------------------------------------------------------------------------
void Args::operator += ( char argPart )
{
    ASSERT( !m_Finalized );
    m_Args += argPart;
}

// Append
//------------------------------------------------------------------------------
void Args::Append( const char * begin, size_t count )
{
    ASSERT( !m_Finalized );
    m_Args.Append( begin, count );
}

// AddDelimiter
//------------------------------------------------------------------------------
void Args::AddDelimiter()
{
    ASSERT( !m_Finalized );

    // Take note of delimiter position in case we need to replace it later
    m_DelimiterIndices.Append( (uint32_t)m_Args.GetLength() );

    m_Args += ' '; // Construct with spaces by default
}

// Clear
//------------------------------------------------------------------------------
void Args::Clear()
{
    ASSERT( !m_Finalized ); // NOTE: Can only reset args before we finalize
    m_Args.Clear();
    m_DelimiterIndices.Clear();
}

// Finalize
//------------------------------------------------------------------------------
bool Args::Finalize( const AString & exe, const AString & nodeNameForError, bool canUseResponseFile )
{
    ASSERT( !m_Finalized );

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        #if defined( __WINDOWS__ )
            // Windows has a 32KiB (inc null terminator) command line length limit with CreateProcess
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
            const uint32_t argLimit( 32767 );
        #elif defined( __OSX__ )
            const uint32_t argLimit( ARG_MAX - 1 );
        #endif

        // Calculate final length of args (including exe name)
        const uint32_t exeLen = exe.GetLength();
        const uint32_t extraLen = 3; // quotes around exe name and space
        const uint32_t argLen = m_Args.GetLength();

        // We need to consider the executable, quotes around the exe and a space
        // as well as the args: "%exe%" %args%
        const uint32_t totalLen = ( argLen + exeLen + extraLen );

        // Small enough?
        if ( totalLen < argLimit )
        {
            #if defined( ASSERTS_ENABLED )
                m_Finalized = true;
            #endif
            return true; // Ok to proceed
        }

        // Args are too long. Can we cope using a Response File?
        if ( canUseResponseFile )
        {
            // Handle per-line limit within response files (e.g. link.exe)
            #if defined( __WINDOWS__ )
                if ( argLen >= 131071 ) // From LNK1170
                {
                    // Change spaces to carriage returns
                    for ( uint32_t i : m_DelimiterIndices )
                    {
                        ASSERT( m_Args[ i ] == ' ' );
                        m_Args[ i ] = '\n';
                    }
                }
            #endif

            #if defined( ASSERTS_ENABLED )
                m_Finalized = true;
            #endif

            // Write args to response file
            {
                PROFILE_SECTION( "CreateResponseFile" )
                m_ResponseFile.Create( *this );
            }

            // Create new args referencing response file
            m_ResponseFileArgs = "@\"";
            m_ResponseFileArgs += m_ResponseFile.GetResponseFilePath();
            m_ResponseFileArgs += "\"";

            return true; // Ok to proceed
        }

        // Need response file but not supported
        FLOG_ERROR( "FBuild: Error: Command Line Limit Exceeded (len: %u, limit: %u) '%s'\n", argLen, argLimit, nodeNameForError.Get() );
        return false;
    #elif defined( __LINUX__ )
        (void)exe;
        (void)nodeNameForError;
        (void)canUseResponseFile;
        // TODO:LINUX Difficult to reliably determine this due to complex interaction with environment
        #if defined( ASSERTS_ENABLED )
            m_Finalized = true;
        #endif
        return true; // Ok to proceed
    #endif
}

// Generate
//------------------------------------------------------------------------------
/*static*/ void Args::StripQuotes( const char * start, const char * end, AString & out )
{
    ASSERT( start );
    ASSERT( end );

    // handle empty inputs
    if ( start == end )
    {
        out.Clear();
        return;
    }

    // strip first quote if there is one
    const char firstChar = *start;
    if ( ( firstChar == '"' ) || ( firstChar == '\'' ) )
    {
        ++start;
    }

    // strip first quote if there is one
    const char lastChar = *( end - 1 );
    if ( ( lastChar == '"' ) || ( lastChar == '\'' ) )
    {
        --end;
    }

    // handle invalid strings (i.e. just one quote)
    if ( end < start )
    {
        out.Clear();
        return;
    }

    // assign unquoted string (could be empty, and that's ok)
    out.Assign( start, end );
}

//------------------------------------------------------------------------------
