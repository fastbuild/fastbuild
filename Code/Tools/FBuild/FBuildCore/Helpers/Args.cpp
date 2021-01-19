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
    , m_DisableResponseFileWrite( false )
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
bool Args::Finalize( const AString & exe, const AString & nodeNameForError, ArgsResponseFileMode responseFileMode )
{
    ASSERT( !m_Finalized );
    #if defined( ASSERTS_ENABLED )
        m_Finalized = true;
    #endif

    // Calculate final length of args (including exe name)
    const uint32_t exeLen = exe.GetLength();
    const uint32_t extraLen = 3; // quotes around exe name and space
    const uint32_t argLen = m_Args.GetLength();

    // We need to consider the executable, quotes around the exe and a space
    // as well as the args: "%exe%" %args%
    const uint32_t totalLen = ( argLen + exeLen + extraLen );

    #if defined( __WINDOWS__ )
        // Windows has a 32KiB (inc null terminator) command line length limit with CreateProcess
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
        const uint32_t argLimit( 32767 );
    #elif defined( __OSX__ )
        const uint32_t argLimit( ARG_MAX - 1 );
    #elif defined( __LINUX__ )
        // On Linux it's problematic to reliably determine this, so we make a best guess
        const uint32_t argLimit( ( 128 * 1024 ) - 1 );
    #endif

    // If the args exceed the cmd line limit, a response file is required
    const bool needResponseFile = ( totalLen >= argLimit );


    if ( needResponseFile )
    {
        switch ( responseFileMode )
        {
            case ArgsResponseFileMode::NEVER:
            {
                // Need response file but not supported
                FLOG_ERROR( "FBuild: Error: Command Line Limit Exceeded (len: %u, limit: %u) '%s'\n", argLen, argLimit, nodeNameForError.Get() );
                return false;
            }
            case ArgsResponseFileMode::IF_NEEDED:   break; // Create below
            case ArgsResponseFileMode::ALWAYS:      break; // Create below
        }
    }
    else
    {
        switch ( responseFileMode )
        {
            case ArgsResponseFileMode::NEVER:       return true;
            case ArgsResponseFileMode::IF_NEEDED:   return true;
            case ArgsResponseFileMode::ALWAYS:      break; // Create below
        }
    }

    // Create response file

    // Handle per-line limit within response files (e.g. link.exe)
    #if defined( __WINDOWS__ )
        // TODO:C Should this be the behavior on all platforms?
        // TODO:C Should this be the behavior regardless of length?
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

    // Write args to response file
    if ( m_DisableResponseFileWrite == false ) // Used by tests
    {
        PROFILE_SECTION( "CreateResponseFile" );
        if ( m_ResponseFile.Create( *this ) == false )
        {
            return false; // Create will have emitted an error
        }
    }

    // Create new args referencing response file
    m_ResponseFileArgs = "@\"";
    m_ResponseFileArgs += m_ResponseFile.GetResponseFilePath();
    m_ResponseFileArgs += "\"";

    return true; // Ok to proceed
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
