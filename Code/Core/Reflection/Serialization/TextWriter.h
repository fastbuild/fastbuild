// TextWriter.h
//------------------------------------------------------------------------------
#pragma once

#if defined( __WINDOWS__ )

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class IOStream;
class RefObject;
class ReflectionInfo;
class ReflectedProperty;
class Struct;

// TextWriter
//------------------------------------------------------------------------------
class TextWriter
{
public:
    explicit TextWriter( IOStream & stream );

    void Write( const RefObject * object );
    void Write( const Struct * str, const ReflectionInfo * info );
private:
    void WriteObject( const RefObject * object );
    void WriteStruct( const void * str, const ReflectionInfo * info );
    void WriteRef( const void * base, const ReflectedProperty * property );
    void WriteArray( const void * base, const ReflectedProperty * property );
    void WriteArrayOfStruct( const void * base, const ReflectedProperty * property );
    void WriteProperties( const void * base, const ReflectionInfo * info );
    void Write( const void * base, const ReflectedProperty * property );
    void Write( const AString & buffer );
    void Write( const char * format, ... ) FORMAT_STRING( 2, 3 );

    inline void Indent() { m_Indent++; }
    inline void Unindent() { ASSERT( m_Indent ); m_Indent--; }

    IOStream * m_Stream;
    size_t m_Indent;
};

//------------------------------------------------------------------------------
#endif // __WINDOWS__

//------------------------------------------------------------------------------
