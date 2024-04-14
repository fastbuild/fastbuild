// BFFMacros
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFMacros.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFMacros::BFFMacros()
    : m_Tokens( 8 )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFMacros::~BFFMacros() = default;

// IsDefined
//------------------------------------------------------------------------------
bool BFFMacros::IsDefined( const AString & token ) const
{
    // user defined tokens
    if ( m_Tokens.Find( token ) )
    {
        return true;
    }

    // fallback to predefined tokens
    #if defined( __WINDOWS__ )
        if ( token == "__WINDOWS__" )
        {
            return true;
        }
    #endif
    #if defined( __LINUX__ )
        if ( token == "__LINUX__" )
        {
            return true;
        }
    #endif
    #if defined( __OSX__ )
        if ( token == "__OSX__" )
        {
            return true;
        }
    #endif
    #if defined( __x86_64__ ) || defined( _M_X64 ) // X64
        if ( token == "__X64__" )
        {
            return true;
        }
    #endif
    #if defined( __aarch64__ ) || defined( _M_ARM64 ) // ARM
        if ( token == "__ARM64__" )
        {
            return true;
        }
    #endif

    return false;
}

// Define
//------------------------------------------------------------------------------
bool BFFMacros::Define( const AString & token )
{
    if ( IsDefined( token ) )
    {
        // trying to overwrite an existing token
        return false;
    }
    else
    {
        m_Tokens.Append( token );
        return true;
    }
}

// Undefine
//------------------------------------------------------------------------------
bool BFFMacros::Undefine( const AString & token )
{
    AString * const defined = m_Tokens.Find( token );
    if ( defined == nullptr )
    {
        // trying to remove a non-existent or predefined token :
        return false;
    }
    else
    {
        m_Tokens.Erase( defined );
        return true;
    }
}

//------------------------------------------------------------------------------
