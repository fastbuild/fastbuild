// TextWriter.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#if defined( __WINDOWS__ )
#include "TextWriter.h"
#include "Core/Containers/Ref.h"
#include "Core/FileIO/IOStream.h"
#include "Core/Math/Mat44.h"
#include "Core/Math/Vec2.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"
#include "Core/Reflection/Object.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/RefObject.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// system
#include <stdarg.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
TextWriter::TextWriter( IOStream & stream )
    : m_Stream( &stream )
    , m_Indent( 0 )
{
}

// Write (Object)
//------------------------------------------------------------------------------
void TextWriter::Write( const RefObject * object )
{
    WriteObject( object );

    // null terminate to allow easy printing to tty
    m_Stream->Write( "\0", 1 );
}

//
//------------------------------------------------------------------------------
void TextWriter::Write( const Struct * str, const ReflectionInfo * info )
{
    WriteStruct( str, info );

    // null terminate to allow easy printing to tty
    m_Stream->Write( "\0", 1 );
}

// WriteObject
//------------------------------------------------------------------------------
void TextWriter::WriteObject( const RefObject * object )
{
    const ReflectionInfo * info = object->GetReflectionInfoV();
    const char * objectTypeStr = info->GetTypeName();

    Object * asObject = DynamicCast< Object >( object );
    if ( asObject )
    {
        const char * objectName = asObject->GetName().Get();
        uint32_t objectId = asObject->GetId();

        // Object start
        Write( "object %s 0x%8.8x \"%s\"", objectTypeStr, objectId, objectName );
    }
    else
    {
        // RefObject start
        Write( "object %s", objectTypeStr );
    }

    WriteProperties( object, info );
}

// WriteStruct
//------------------------------------------------------------------------------
void TextWriter::WriteStruct( const void * str, const ReflectionInfo * info )
{
    const char * structTypeStr = info->GetTypeName();

    // object start
    Write( "struct %s", structTypeStr );

    WriteProperties( str, info );
}

// WritePtr
//------------------------------------------------------------------------------
void TextWriter::WriteRef( const void * base, const ReflectedProperty * property )
{
    Ref< RefObject > ref;
    property->GetProperty( base, &ref );
    const RefObject * object( ref.Get() );
    const ReflectionInfo * info( object ? object->GetReflectionInfoV() : nullptr );
    const char * refTypeName = info ? info->GetTypeName() : "null";
    Write( "%-6s %-16s %s", "ref", property->GetName(), refTypeName );
    if ( object )
    {
        WriteProperties( object, info );
    }
}

// WriteArray
//------------------------------------------------------------------------------
void TextWriter::WriteArray( const void * base, const ReflectedProperty * property )
{
    const char * arrayName = property->GetName();
    const char * elementType = ReflectedProperty::TypeToTypeString( property->GetType() );

    Write( "array< %s > %s", elementType, arrayName );
    Write( "{" );
    Indent();

    // get access to the array
    void * arrayAddr = (void *)((size_t)base + property->GetOffset() );
    Array< void * > * array = static_cast< Array< void * > * >( arrayAddr );

    // get access to the array data
    size_t arrayDataBegin = (size_t)array->Begin();
    size_t arrayDataEnd = (size_t)array->End();

    // determine the number of elements
    const size_t arrayElementSize = property->GetPropertySize();
    size_t numElements = ( arrayDataEnd - arrayDataBegin ) / arrayElementSize;
    ASSERT( ( ( arrayDataEnd - arrayDataBegin ) % arrayElementSize ) == 0 ); // Sanity check

    // write each element
    for ( size_t i=0; i<numElements; ++i )
    {
        // create a temp ReflectedProperty for the element
        const size_t offset( i * arrayElementSize );
        ReflectedProperty arrayElement( "element", (uint32_t)offset, property->GetType(), false );

        Write( (void *)arrayDataBegin, &arrayElement );
    }

    Unindent();
    Write( "}" );
}

// WriteArrayOfStruct
//------------------------------------------------------------------------------
void TextWriter::WriteArrayOfStruct( const void * base, const ReflectedProperty * property )
{
    const ReflectedPropertyStruct * rps = static_cast< const ReflectedPropertyStruct * >( property );

    const char * arrayName = property->GetName();
    const char * structType = rps->GetStructReflectionInfo()->GetTypeName();

    Write( "arrayOfStruct< %s > %s", structType, arrayName );
    Write( "{" );
    Indent();

    // get access to the array
    void * arrayAddr = (void *)((size_t)base + property->GetOffset() );
    Array< void * > * array = static_cast< Array< void * > * >( arrayAddr );

    // get access to the array data
    size_t arrayDataBegin = (size_t)array->Begin();
    size_t arrayDataEnd = (size_t)array->End();

    // determine the number of elements
    const size_t arrayElementSize = property->GetPropertySize();
    size_t numElements = ( arrayDataEnd - arrayDataBegin ) / arrayElementSize;
    ASSERT( ( ( arrayDataEnd - arrayDataBegin ) % arrayElementSize ) == 0 ); // Sanity check

    // write each element
    for ( size_t i=0; i<numElements; ++i )
    {
        // create a temp ReflectedProperty for the element
        const size_t offset( i * arrayElementSize );
        ReflectedProperty arrayElement( "element", (uint32_t)offset, property->GetType(), false );

        // Write each struct
        WriteStruct( (void *)( arrayDataBegin + offset ), rps->GetStructReflectionInfo() );
    }

    Unindent();
    Write( "}" );
}

// WriteProperties
//------------------------------------------------------------------------------
void TextWriter::WriteProperties( const void * base, const ReflectionInfo * info )
{
    Write( "{" );
    Indent();

    while ( info )
    {
        // write properties
        ReflectionIter end = info->End();
        for ( ReflectionIter it = info->Begin();
              it != end;
              ++it )
        {
            const ReflectedProperty & property = *it;
            if ( property.IsArray() )
            {
                if ( property.GetType() == PT_STRUCT )
                {
                    WriteArrayOfStruct( base, &property );
                }
                else
                {
                    WriteArray( base, &property );
                }
            }
            else if ( property.GetType() == PT_STRUCT )
            {
                const ReflectedPropertyStruct & str = (const ReflectedPropertyStruct &)property;
                const void * structBase = str.GetStructBase( base );
                const ReflectionInfo * structInfo = str.GetStructReflectionInfo();
                WriteStruct( structBase, structInfo );
            }
            else if ( property.GetType() == PT_REF )
            {
                WriteRef( base, &property );
            }
            else
            {
                Write( base, &property );
            }
        }

        info = info->GetSuperClass();
    }

    Unindent();
    Write( "}" );
}

// Write (Property)
//------------------------------------------------------------------------------
void TextWriter::Write( const void * base, const ReflectedProperty * property )
{
    const char * type = property->GetTypeString();
    const char * name = property->GetName();
    AStackString<> value;
    property->ToString( base, value );

    Write( "%-6s %-16s %s", type, name, value.Get() );
}

// Write
//------------------------------------------------------------------------------
void TextWriter::Write( const AString & buffer )
{
    Write( buffer.Get() );
}

// Write
//------------------------------------------------------------------------------
void TextWriter::Write( const char * format, ... )
{
    // do user formatting
    va_list arglist;
    va_start( arglist, format );
    AStackString<> buffer;
    buffer.VFormat( format, arglist );
    va_end( arglist );

    // work out indent
    const size_t MAX_INDENT( 16 );
    static const char tabBuffer[] = { "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" };
    static_assert( sizeof( tabBuffer ) == MAX_INDENT + 1, "Unexpected sizeof(tabBuffer)" ); // MAX_INDENT + 0 terminator
    size_t indent = m_Indent > MAX_INDENT ? MAX_INDENT : m_Indent;
    const char * tabs = &tabBuffer[ MAX_INDENT - indent ];

    // combine
    AStackString<> finalBuffer;
    finalBuffer.Format( "%s%s\n", tabs, buffer.Get() );

    // serialize to stream
    m_Stream->Write( finalBuffer.Get(), finalBuffer.GetLength() );
}

//------------------------------------------------------------------------------
#endif // __WINDOWS__
