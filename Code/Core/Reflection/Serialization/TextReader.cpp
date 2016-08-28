// TextReader.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "TextReader.h"
#include "Core/Containers/Ref.h"
#include "Core/Containers/WeakRef.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/Math/Mat44.h"
#include "Core/Math/Vec2.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"
#include "Core/Reflection/Container.h"
#include "Core/Reflection/Object.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/RefObject.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// system
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Helpers
//------------------------------------------------------------------------------
namespace
{
    template< typename T >
    bool ReadPropertyFromString( void * base,
                                 const ReflectedProperty * p,
                                 const AString & propertyValue,
                                 bool isArray )
    {
        T actualValue; // TODO: This temporary could probably be eliminated

        if ( !ReflectedProperty::FromString( propertyValue, &actualValue ) )
        {
            return false;
        }

        if ( isArray )
        {
            Array< T > *array = (Array< T > *)( (size_t)base + p->GetOffset() );
            array->Append( actualValue );
        }
        else
        {
            p->SetProperty( base, actualValue );
        }
        return true; // whether property set or not
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
TextReader::TextReader( ConstMemoryStream & stream )
    : m_Pos( (const char *)stream.GetData() )
    , m_End( (const char *)( (size_t)stream.GetData() + stream.GetSize() ) )
    , m_RootObject( nullptr )
    , m_DeserializationStack( 32, true )
    , m_ObjectsToInit( 32, true )
    , m_UnresolvedWeakRefs( 32, true )
{
    // Parsing logic assumed a null-terminated stream
    ASSERT( m_End[ -1 ] == '\000' );
}

// Read (Object)
//------------------------------------------------------------------------------
RefObject * TextReader::Read()
{
    // Load
    if ( ReadLines() == false )
    {
        return nullptr;
    }

    if ( m_RootObject )
    {
        Object * obj = DynamicCast< Object >( m_RootObject );
        ASSERT( obj );
        ReflectionInfo::RegisterRootObject( obj );
    }

    // ResolveWeakRefs
    ResolveWeakRefs();

    // Init
    auto end = m_ObjectsToInit.End();
    for ( auto it=m_ObjectsToInit.Begin(); it!=end; ++it )
    {
        ( *it )->Init();
    }

    return m_RootObject;
}

// ReadLines
//------------------------------------------------------------------------------
bool TextReader::ReadLines()
{
    for ( ;; )
    {
        // Skip Leading
        AStackString<> token;
        if ( !GetToken( token ) )
        {
            return true;
        }

        // object?
        if ( token == "object" )
        {
            if ( !ReadObject() )
            {
                return false;
            }
            continue;
        }

        // struct?
        if ( token == "struct" )
        {
            if ( !ReadStruct() )
            {
                return false;
            }
            continue;
        }

        // ref?
        if ( token == "ref" )
        {
            if ( !ReadRef() )
            {
                return false;
            }
            continue;
        }

        // children?
        if ( token == "children" )
        {
            if ( !ReadChildren() )
            {
                return false;
            }
            continue;
        }

        // begin object properties?
        if ( token == "{" )
        {
            continue;
        }

        // end object/struct properties?
        if ( token == "}" )
        {
            m_DeserializationStack.Pop();
            continue;
        }

        // assume property
        PropertyType pt;
        pt = GetPropertyTypeFromString( token );
        if ( pt != PT_NONE )
        {
            if ( !ReadProperty( pt ) )
            {
                return false;
            }
            continue;
        }

        if ( token == "weakRef" )
        {
            if ( !ReadWeakRef() )
            {
                return false;
            }
            continue;
        }

        if ( token == "array<" )
        {
            if ( !ReadArray() )
            {
                return false;
            }
            continue;
        }

        if ( token == "arrayOfStruct<" )
        {
            if ( !ReadArrayOfStruct() )
            {
                return false;
            }
            continue;
        }

        Error( "Unkown token" );
        return false;
    }
}

// ReadObject
//------------------------------------------------------------------------------
bool TextReader::ReadObject()
{
    // Type
    AStackString<> type;
    if ( !GetToken( type ) )
    {
        Error( "Missing type token" );
        return false;
    }

    // ID
    AStackString<> id;
    if ( !GetToken( id ) )
    {
        Error( "Missing id token" );
        return false;
    }

    // Name
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing name" );
        return false;
    }

    // Create Object
    RefObject * refObj = ReflectionInfo::CreateObject( type );
    Object * obj = DynamicCast< Object >( refObj );
    ASSERT( obj ); // TODO: handle this gracefully (skip)
    if ( obj == nullptr )
    {
        Error( "Failed to create object" );
        return false;
    }
    m_ObjectsToInit.Append( obj );

    // set name
    name.Replace( "\"", "" ); // strip quotes
    obj->SetName( name );

    // add to parent if not root
    if ( m_DeserializationStack.IsEmpty() == false )
    {
        // get parent container
        RefObject * topRefObject = (RefObject *)( m_DeserializationStack.Top().m_Base );
        Container * parentContainer = DynamicCast< Container >( topRefObject );
        ASSERT( parentContainer ); // parent is not a container - impossible!

        // sanity check child
        Object * child = DynamicCast< Object >( obj );
        ASSERT( child ); // child is not an Object - impossible!

        // add to parent
        parentContainer->AddChild( child );
    }

    // Push onto stack
    StackFrame sf;
    sf.m_Base = obj;
    sf.m_Reflection = obj->GetReflectionInfoV();
    sf.m_ArrayProperty = nullptr;
    #ifdef DEBUG
        sf.m_RefObject = obj;
        sf.m_Struct = nullptr;
    #endif
    m_DeserializationStack.Append( sf );

    if ( m_RootObject == nullptr )
    {
        m_RootObject = obj;
    }

    return true;
}

// ReadStruct
//------------------------------------------------------------------------------
bool TextReader::ReadStruct()
{
    // Name
    AStackString<> structName;
    if ( !GetToken( structName ) )
    {
        Error( "Missing structName token" );
        return false;
    }

    // get ReflectedProperty for stuct
    const StackFrame & current = m_DeserializationStack.Top();

    // for an array, retrieve the object ReflectionInfo
    const bool isArray = ( current.m_ArrayProperty != nullptr );
    if ( isArray )
    {
        // ReadArrayOfStruct has already ensured we're in sync
        ASSERT( current.m_Base );
        ASSERT( current.m_Reflection );

        // Get Reflection info
        const ReflectedProperty * rp = current.m_ArrayProperty;
        ASSERT( rp->IsArray() );
        ASSERT( rp->GetType() == PT_STRUCT );
        const ReflectedPropertyStruct * rps = static_cast< const ReflectedPropertyStruct * >( rp );

        // get existing array size
        const ReflectionInfo * structRI = rps->GetStructReflectionInfo();
        const size_t oldSize = rps->GetArraySize( current.m_Base );

        // add one element
        rps->ResizeArrayOfStruct( current.m_Base, oldSize + 1 );
        Struct * newStruct = rps->GetStructInArray( current.m_Base, oldSize );

        // Push struct onto stack
        StackFrame sf;
        sf.m_Base = newStruct;
        sf.m_Reflection = structRI;
        sf.m_ArrayProperty = nullptr;
        #ifdef DEBUG
            sf.m_RefObject = nullptr;
            sf.m_Struct = newStruct;
        #endif
        m_DeserializationStack.Append( sf );
    }
    else
    {
        ASSERT( current.m_Reflection );
        const ReflectedProperty * rp = current.m_Reflection->GetReflectedProperty( structName );
        if ( rp == nullptr )
        {
            // struct no longer exists
            Error( "Unable to update struct" );
            return false; // TODO: Handle this (skip mode), return true
        }

        if ( rp->IsArray() )
        {
            // changed to an array of something
            Error( "Unexpected Array" );
            return false; // TODO: Handle this (skip mode), return true
        }


        if ( rp->GetType() != PT_STRUCT )
        {
            // changed from struct to another type
            Error( "Property is no longer a struct" );
            return false; // TODO: Handle this (skip mode), return true
        }

        // Get Reflection info
        const ReflectedPropertyStruct * rps = (ReflectedPropertyStruct *)rp;
        const ReflectionInfo * structInfo = rps->GetStructReflectionInfo();
        ASSERT( structInfo );

        // Push struct onto stack
        StackFrame sf;
        sf.m_Base = (void *)( (size_t)m_DeserializationStack.Top().m_Base + rp->GetOffset() );
        sf.m_Reflection = structInfo;
        sf.m_ArrayProperty = nullptr;
        #ifdef DEBUG
            sf.m_RefObject = nullptr;
            sf.m_Struct = (Struct *)sf.m_Base;
        #endif
        m_DeserializationStack.Append( sf );
    }

    return true;
}

// ReadRef
//------------------------------------------------------------------------------
bool TextReader::ReadRef()
{
    // Name
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing ref name" );
        return false;
    }

    AStackString<> value;
    if ( !GetString( value ) )
    {
        Error( "Missing ref value" );
        return false;
    }

    Ref< RefObject > ref;
    if ( value != "null" )
    {
        ref = (RefObject *)ReflectionInfo::CreateObject( value );
        if ( ref.Get() == nullptr )
        {
            Error( "Unable to create sub-object" );
            ASSERT( false ); // TODO: Handle failure/skipping gracefully
            return false;
        }
    }

    const StackFrame & sf = m_DeserializationStack.Top();

    if ( sf.m_ArrayProperty )
    {
        void * arrayBase = (void *)( (size_t)sf.m_Base + (size_t)sf.m_ArrayProperty->GetOffset() );
        Array< Ref< RefObject > > *array = ( Array< Ref< RefObject > > *)( arrayBase );
        array->Append( ref );
    }
    else
    {
        sf.m_Reflection->SetProperty( sf.m_Base, name.Get(), ref );
    }

    // do sub-object properties if needed
    if ( ref.Get() )
    {
        StackFrame newFrame;
        newFrame.m_Base = (void *)ref.Get();
        newFrame.m_Reflection = ref->GetReflectionInfoV();
        newFrame.m_ArrayProperty = nullptr;
        #ifdef DEBUG
            newFrame.m_RefObject = ref.Get();
            newFrame.m_Struct = nullptr;
        #endif
        m_DeserializationStack.Append( newFrame );
    }

    return true;
}

// ReadWeakRef
//------------------------------------------------------------------------------
bool TextReader::ReadWeakRef()
{
    // Name
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing weakref name" );
        return false;
    }

    AStackString<> value;
    if ( !GetString( value ) )
    {
        Error( "Missing weakref value" );
        return false;
    }

    const StackFrame & sf = m_DeserializationStack.Top();

    // null weak refs can be taken care of right now
    if ( value == "null" )
    {
        WeakRef< Object > nullWR;
        sf.m_Reflection->SetProperty( sf.m_Base, name.Get(), nullWR );
        return true;
    }

    // non-null WeakRefs must be deferred
    // (since we might not have created the child object yet)
    UnresolvedWeakRef u;
    u.m_Base = sf.m_Base;
    u.m_Reflection = sf.m_Reflection;
    u.m_WeakRefName = name;
    u.m_WeakRefValue = value;
    m_UnresolvedWeakRefs.Append( u );

    return true;
}

// ReadArray
//------------------------------------------------------------------------------
bool TextReader::ReadArray()
{
    // element type
    AStackString<> elementType;
    if ( !GetToken( elementType ) )
    {
        Error( "Missing array element type" );
        return false;
    }

    // close array >
    SkipWhitespace();
    if ( *m_Pos != '>' )
    {
        Error( "Missing array close token >" );
        return false;
    }
    m_Pos++;

    // property name
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing array property name" );
        return false;
    }

    // Get Array ReflectedProperty
    const StackFrame & sf = m_DeserializationStack.Top();
    const ReflectedProperty * rp = sf.m_Reflection->GetReflectedProperty( name );
    if ( !rp )
    {
        Error( "missing array" ); // TODO: This should be handled gracefully (skip)
        return false;
    }
    if ( rp->IsArray() == false )
    {
        Error( "property is not an array" ); // TODO: This should be handled gracefully (skip)
        return false;
    }

    StackFrame newSF;
    newSF.m_Base = sf.m_Base;
    newSF.m_Reflection = sf.m_Reflection;
    newSF.m_ArrayProperty = rp;
    #ifdef DEBUG
        newSF.m_RefObject = nullptr;
        newSF.m_Struct = nullptr;
    #endif
    m_DeserializationStack.Append( newSF );

    return true;
}

// ReadArrayOfStruct
//------------------------------------------------------------------------------
bool TextReader::ReadArrayOfStruct()
{
    // element type
    AStackString<> elementType;
    if ( !GetToken( elementType ) )
    {
        Error( "Missing array element type" );
        return false;
    }

    // close array >
    SkipWhitespace();
    if ( *m_Pos != '>' )
    {
        Error( "Missing array close token >" );
        return false;
    }
    m_Pos++;

    // property name
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing array property name" );
        return false;
    }

    // Get Array ReflectedProperty
    const StackFrame & sf = m_DeserializationStack.Top();
    const ReflectedProperty * rp = sf.m_Reflection->GetReflectedProperty( name );
    if ( !rp )
    {
        Error( "missing array" ); // TODO: This should be handled gracefully (skip)
        return false;
    }
    if ( rp->IsArray() == false )
    {
        Error( "property is not an array" ); // TODO: This should be handled gracefully (skip)
        return false;
    }
    if ( rp->GetType() != PT_STRUCT )
    {
        Error( "property is not an arrayOfStruct" ); // TODO: This should be handled gracefully (skip)
        return false;
    }

    const ReflectedPropertyStruct * rps = static_cast< const ReflectedPropertyStruct * >( rp );
    if ( elementType != rps->GetStructReflectionInfo()->GetTypeName() )
    {
        Error( "mismatched struct type in arrayOfStruct" ); // TODO: This should be handled gracefully (skip)
        return false;
    }

    // clear the array (it will be re-populated)
    rps->ResizeArrayOfStruct( sf.m_Base, 0 );

    StackFrame newSF;
    newSF.m_Base = sf.m_Base;
    newSF.m_Reflection = sf.m_Reflection;
    newSF.m_ArrayProperty = rps;
    #ifdef DEBUG
        newSF.m_RefObject = nullptr;
        newSF.m_Struct = nullptr;
    #endif
    m_DeserializationStack.Append( newSF );

    return true;
}

// ReadChildren
//------------------------------------------------------------------------------
bool TextReader::ReadChildren()
{
    // add parent scope for children
    Object * parentObject = m_ObjectsToInit.Top();
    ASSERT( DynamicCast< Container >( parentObject ) );

    StackFrame parentScope;
    parentScope.m_Base = parentObject;
    parentScope.m_Reflection = parentObject->GetReflectionInfoV();
    parentScope.m_ArrayProperty = nullptr;
    #ifdef DEBUG
        parentScope.m_RefObject = parentObject;
        parentScope.m_Struct = nullptr;
    #endif
    m_DeserializationStack.Append( parentScope );

    return true;
}

// ReadProperty
//------------------------------------------------------------------------------
bool TextReader::ReadProperty( PropertyType propertyType )
{
    AStackString<> name;
    if ( !GetToken( name ) )
    {
        Error( "Missing property name token" );
        return false;
    }

    AStackString<> value;
    if ( !GetString( value ) )
    {
        Error( "Missing property value" );
        return false;
    }

    if ( m_DeserializationStack.IsEmpty() )
    {
        Error( "Property seen with no object" );
        return false;
    }

    StackFrame * sf = &m_DeserializationStack.Top();

    const bool isArray = ( sf->m_ArrayProperty != nullptr );

    bool ok = false;

    const ReflectedProperty * p = nullptr;

    // for an array, retrieve the object ReflectionInfo
    if ( isArray )
    {
        ASSERT( sf->m_ArrayProperty ); // shouldn't have gotten here
        p = sf->m_ArrayProperty;
    }
    else
    {
        p = sf->m_Reflection->GetReflectedProperty( name );;
        if ( p == nullptr )
        {
            return true; // failure to set property is not an error
        }
    }

    void * base = sf->m_Base;

    switch ( propertyType )
    {
        case PT_NONE:       ASSERT( false ); return false;
        case PT_FLOAT:      ok = ReadPropertyFromString< float >( base, p, value, isArray ); break;
        case PT_UINT8:      ok = ReadPropertyFromString< uint8_t >( base, p, value, isArray ); break;
        case PT_UINT16:     ok = ReadPropertyFromString< uint16_t >( base, p, value, isArray ); break;
        case PT_UINT32:     ok = ReadPropertyFromString< uint32_t >( base, p, value, isArray ); break;
        case PT_UINT64:     ok = ReadPropertyFromString< uint64_t >( base, p, value, isArray ); break;
        case PT_INT8:       ok = ReadPropertyFromString< int8_t >( base, p, value, isArray ); break;
        case PT_INT16:      ok = ReadPropertyFromString< int16_t >( base, p, value, isArray ); break;
        case PT_INT32:      ok = ReadPropertyFromString< int32_t >( base, p, value, isArray ); break;
        case PT_INT64:      ok = ReadPropertyFromString< int64_t >( base, p, value, isArray ); break;
        case PT_BOOL:       ok = ReadPropertyFromString< bool >( base, p, value, isArray ); break;
        case PT_ASTRING:    ok = ReadPropertyFromString< AString >( base, p, value, isArray ); break;
        case PT_VEC2:       ok = ReadPropertyFromString< Vec2 >( base, p, value, isArray ); break;
        case PT_VEC3:       ok = ReadPropertyFromString< Vec3 >( base, p, value, isArray ); break;
        case PT_VEC4:       ok = ReadPropertyFromString< Vec4 >( base, p, value, isArray ); break;
        case PT_MAT44:      ok = ReadPropertyFromString< Mat44 >( base, p, value, isArray ); break;
        case PT_REF:        ASSERT( false ); return false; // Should not get here
        case PT_WEAKREF:    ASSERT( false ); return false; // Should not get here
        case PT_STRUCT:     ASSERT( false ); return false; // Should not get here
    }

    if ( !ok )
    {
        Error( "Failed to read property" );
    }

    return ok;
}

// SkipWhitespace
//------------------------------------------------------------------------------
void TextReader::SkipWhitespace( bool stopAtLineEnd )
{
loop:
    const char c = *m_Pos;
    if ( ( c == ' ' ) || ( c == '\t' ) )
    {
        m_Pos++;
        goto loop;
    }
    if ( ( c == '\r' ) || ( c == '\n' ) )
    {
        if ( !stopAtLineEnd )
        {
            m_Pos++;
            goto loop;
        }
    }
}

// GetToken
//------------------------------------------------------------------------------
bool TextReader::GetToken( AString & token )
{
    SkipWhitespace();

    const char * tokenStart = m_Pos;
loop:
    const char c = *m_Pos;
    if ( ( c != ' ' )  &&
         ( c != '\t' ) &&
         ( c != '\r' ) &&
         ( c != '\n' ) &&
         ( c != 0 ) )
    {
        ++m_Pos;
        goto loop;
    }

    const char * tokenEnd = m_Pos;
    if ( tokenEnd == tokenStart )
    {
        return false;
    }
    token.Assign( tokenStart, tokenEnd );
    return true;
}


// GetString
//------------------------------------------------------------------------------
bool TextReader::GetString( AString & string )
{
    SkipWhitespace( true ); // don't go past end of line

    bool quoted = ( *m_Pos == '"' );
    if ( quoted )
    {
        m_Pos++;
    }
    bool escapeActive = false;

    for ( ;; )
    {
        const char c = *m_Pos;
        ++m_Pos;
        if ( quoted )
        {
            // this character is escaped, so just use it as is
            if ( escapeActive == false )
            {
                // do we want to escape next character?
                if ( c == '\\' )
                {
                    escapeActive = true;
                    continue;
                }

                // unescaped closing quote?
                if ( ( c == '"' ) || ( c == 0 ) )
                {
                    break; // all done!
                }
            }
            else
            {
                escapeActive = false;
            }
        }
        else
        {
            // unescaped string terminates at end of line
            if ( ( c == '\r' ) || ( c == '\n' ) || ( c == 0 ) )
            {
                break; // all done!
            }
        }

        string += c;
    }

    if ( quoted )
    {
        m_Pos++;
    }
    return true;
}

// Error
//------------------------------------------------------------------------------
void TextReader::Error( const char * error ) const
{
    // TODO: Better error management
    (void)error;
    ASSERT( false );
}

// ResolveWeakRefs
//------------------------------------------------------------------------------
void TextReader::ResolveWeakRefs() const
{
    auto end = m_UnresolvedWeakRefs.End();
    for ( auto it=m_UnresolvedWeakRefs.Begin(); it!=end; ++it )
    {
        const UnresolvedWeakRef & u = *it;
        WeakRef< Object > weakRef( ReflectionInfo::FindObjectByScopedName( u.m_WeakRefValue ) );
        u.m_Reflection->SetProperty( u.m_Base, u.m_WeakRefName.Get(), weakRef );
    }
}

//------------------------------------------------------------------------------
