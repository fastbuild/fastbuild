// AStackString.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "AString.h"

// AStackString<>
//------------------------------------------------------------------------------
template <int RESERVED = 256>
class AStackString : public AString
{
public:
    explicit AStackString();
    explicit AStackString( const AString & string );
    explicit AStackString( AString && string );
    explicit AStackString( const AStackString & string );
    explicit AStackString( AStackString && string );
    explicit AStackString( const char * string );
    explicit AStackString( const char * start, const char * end );
    ~AStackString() = default;

    AStackString<RESERVED> & operator=( const char * string )
    {
        Assign( string );
        return *this;
    }
    AStackString<RESERVED> & operator=( const AString & string )
    {
        Assign( string );
        return *this;
    }
    AStackString<RESERVED> & operator=( AString && string )
    {
        Assign( Move( string ) );
        return *this;
    }
    AStackString<RESERVED> & operator=( const AStackString & string )
    {
        Assign( string );
        return *this;
    }
    AStackString<RESERVED> & operator=( AStackString && string )
    {
        Assign( Move( string ) );
        return *this;
    }

private:
    char m_Storage[ RESERVED + 1 ];
};

// CTAD template deduction guide
template <int RESERVED = 256>
AStackString() -> AStackString<RESERVED>;
template <int RESERVED = 256>
AStackString( const AStackString<> & ) -> AStackString<RESERVED>;

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString()
{
    ASSERT( m_ReferenceCount == nullptr );
    m_ReferenceCount = nullptr;
    m_Contents = m_Storage;
    SetReserved( RESERVED );
    ASSERT( !IsUsingSharedMemory() );
    m_Storage[ 0 ] = '\0';
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( const AString & string )
    : AStackString()
{
    Assign( string );
}

// CONSTRUCTOR (AString &&)
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( AString && string )
    : AStackString()
{
    Assign( Move( string ) );
}

// CONSTRUCTOR (const AString &)
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( const AStackString & string )
    : AStackString()
{
    Assign( string );
}

// CONSTRUCTOR (AStackString &&)
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( AStackString && string )
    : AStackString()
{
    Assign( Move( string ) );
}

// CONSTRUCTOR (const char *)
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( const char * string )
    : AStackString()
{
    Assign( string );
}

// CONSTRUCTOR( const char *, const char *)
//------------------------------------------------------------------------------
template <int RESERVED>
AStackString<RESERVED>::AStackString( const char * start, const char * end )
    : AStackString()
{
    Assign( start, end );
}

//------------------------------------------------------------------------------
