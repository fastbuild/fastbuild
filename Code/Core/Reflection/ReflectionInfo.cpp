// ReflectionInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "ReflectionInfo.h"
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/CRC32.h"
#include "Core/Process/Process.h"
#include "Core/Reflection/Container.h"
#include "Core/Reflection/Object.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/Serialization/TextReader.h"
#include "Core/Reflection/Serialization/TextWriter.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// System
#include <memory.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ ReflectionInfo * ReflectionInfo::s_FirstReflectionInfo( nullptr );
/*static*/ Array< Object * > ReflectionInfo::s_RootObjects( 8, false );

// CONSTRUCTOR
//------------------------------------------------------------------------------
ReflectionInfo::ReflectionInfo()
    : m_TypeNameCRC( 0 )
    , m_Properties( 0, true )
    , m_SuperClass( nullptr )
    , m_Next( nullptr )
    , m_TypeName( nullptr )
    , m_IsAbstract( false )
    , m_StructSize( 0 )
    , m_MetaDataChain( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ReflectionInfo::~ReflectionInfo() = default;

// Begin
//------------------------------------------------------------------------------
ReflectionIter ReflectionInfo::Begin() const
{
    return ReflectionIter( this, 0 );
}

// End
//------------------------------------------------------------------------------
ReflectionIter ReflectionInfo::End() const
{
    return ReflectionIter( this, (uint32_t)m_Properties.GetSize() );
}

// GetReflectedProperty
//------------------------------------------------------------------------------
const ReflectedProperty & ReflectionInfo::GetReflectedProperty( uint32_t index ) const
{
    return *m_Properties[ index ];
}

// GetReflectedProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::GetReflectedProperty( const AString & propertyName ) const
{
    return FindProperty( propertyName.Get() );
}

// SetProperty
//------------------------------------------------------------------------------
#define GETSET_PROPERTY( getValueType, setValueType ) \
    bool ReflectionInfo::GetProperty( void * object, const char * name, getValueType * value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (getValueType *)nullptr ) ) && !p->IsArray() ) \
        { \
            p->GetProperty( object, value ); \
            return true; \
        } \
        return false; \
    } \
    bool ReflectionInfo::SetProperty( void * object, const char * name, setValueType value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (getValueType *)nullptr ) ) && !p->IsArray() ) \
        { \
            p->SetProperty( object, value ); \
            return true; \
        } \
        return false; \
    }

GETSET_PROPERTY( float, float )
GETSET_PROPERTY( uint8_t, uint8_t )
GETSET_PROPERTY( uint16_t, uint16_t )
GETSET_PROPERTY( uint32_t, uint32_t )
GETSET_PROPERTY( uint64_t, uint64_t )
GETSET_PROPERTY( int8_t, int8_t )
GETSET_PROPERTY( int16_t, int16_t )
GETSET_PROPERTY( int32_t, int32_t )
GETSET_PROPERTY( int64_t, int64_t )
GETSET_PROPERTY( bool, bool )
GETSET_PROPERTY( AString, const AString & )
GETSET_PROPERTY( Vec2, const Vec2 & )
GETSET_PROPERTY( Vec3, const Vec3 & )
GETSET_PROPERTY( Vec4, const Vec4 & )
GETSET_PROPERTY( Mat44, const Mat44 & )
GETSET_PROPERTY( Ref< RefObject >, const Ref< RefObject > & )
GETSET_PROPERTY( WeakRef< Object >, const WeakRef< Object > & )

#define GETSET_PROPERTY_ARRAY( valueType ) \
    bool ReflectionInfo::GetProperty( void * object, const char * name, Array< valueType > * value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (valueType *)nullptr ) ) && p->IsArray() ) \
        { \
            p->GetProperty( object, value ); \
            return true; \
        } \
        return false; \
    } \
    bool ReflectionInfo::SetProperty( void * object, const char * name, const Array< valueType > & value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (valueType *)nullptr ) ) && p->IsArray() ) \
        { \
            p->SetProperty( object, value ); \
            return true; \
        } \
        return false; \
    }

GETSET_PROPERTY_ARRAY( AString )

#undef GETSET_PROPERTY
#undef GETSET_PROPERTY_ARRAY

// BindReflection
//------------------------------------------------------------------------------
/*static*/ void ReflectionInfo::BindReflection( ReflectionInfo & reflectionInfo )
{
    ASSERT( reflectionInfo.m_Next == nullptr );
    reflectionInfo.m_Next = s_FirstReflectionInfo;
    s_FirstReflectionInfo = &reflectionInfo;
}

// SetTypeName
//------------------------------------------------------------------------------
void ReflectionInfo::SetTypeName( const char * typeName )
{
    m_TypeName = typeName;
    m_TypeNameCRC = CRC32::Calc( typeName, AString::StrLen( typeName ) );
}

// HasMetaDataInternal
//------------------------------------------------------------------------------
const IMetaData * ReflectionInfo::HasMetaDataInternal( const ReflectionInfo * ri ) const
{
    const IMetaData * m = m_MetaDataChain;
    while ( m )
    {
        if ( m->GetReflectionInfoV() == ri )
        {
            return m;
        }
        m = m->GetNext();
    }
    return m_SuperClass ? m_SuperClass->HasMetaDataInternal( ri ) : nullptr;
}

// AddPropertyStruct
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyStruct( void * offset, const char * memberName, const ReflectionInfo * structInfo )
{
    ReflectedPropertyStruct * r = new ReflectedPropertyStruct( memberName, (uint32_t)( (size_t)offset ), structInfo );
    m_Properties.Append( r );
}

// AddPropertyArrayOfStruct
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyArrayOfStruct( void * memberOffset, const char * memberName, const ReflectionInfo * structInfo )
{
    ReflectedPropertyStruct * r = new ReflectedPropertyStruct( memberName, (uint32_t)( (size_t)memberOffset ), structInfo, true );
    m_Properties.Append( r );
}

// AddPropertyInternal
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyInternal( PropertyType type, uint32_t offset, const char * memberName, bool isArray )
{
    ReflectedProperty * r = new ReflectedProperty( memberName, offset, type, isArray );
    m_Properties.Append( r );
}

// AddMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddMetaData( IMetaData & metaDataChain )
{
    ASSERT( m_MetaDataChain == nullptr );
    m_MetaDataChain = &metaDataChain;
}

// AddPropertyMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyMetaData( IMetaData & metaDataChain )
{
    m_Properties.Top()->AddMetaData( &metaDataChain );
}

//  FindProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::FindProperty( const char * name ) const
{
    const uint32_t nameCRC = CRC32::Calc( name, AString::StrLen( name ) );
    return FindPropertyRecurse( nameCRC );
}

//  FindProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::FindPropertyRecurse( uint32_t nameCRC ) const
{
    auto end = m_Properties.End();
    for ( auto it = m_Properties.Begin(); it != end; ++it )
    {
        if ( ( *it )->GetNameCRC() == nameCRC )
        {
            return *it;
        }
    }
    if ( m_SuperClass )
    {
        return m_SuperClass->FindPropertyRecurse( nameCRC );
    }
    return nullptr;
}

// CreateObject
//------------------------------------------------------------------------------
/*static*/ RefObject * ReflectionInfo::CreateObject( const AString & objectType )
{
    const uint32_t objectTypeCRC = CRC32::Calc( objectType );
    const ReflectionInfo * ri = s_FirstReflectionInfo;
    while ( ri )
    {
        if ( objectTypeCRC == ri->m_TypeNameCRC )
        {
            return ri->CreateObject();
        }
        ri = ri->m_Next;
    }
    return nullptr;
}

// CreateStruct
//------------------------------------------------------------------------------
/*static*/ Struct * ReflectionInfo::CreateStruct( const AString & structType )
{
    const uint32_t objectTypeCRC = CRC32::Calc( structType );
    const ReflectionInfo * ri = s_FirstReflectionInfo;
    while ( ri )
    {
        if ( objectTypeCRC == ri->m_TypeNameCRC )
        {
            return ri->CreateStruct();
        }
        ri = ri->m_Next;
    }
    return nullptr;
}

// CreateObject
//------------------------------------------------------------------------------
RefObject * ReflectionInfo::CreateObject() const
{
    ASSERT( IsObject() );
    ASSERT( !IsAbstract() );
    return (RefObject *)Create();
}

// CreateStruct
//------------------------------------------------------------------------------
Struct * ReflectionInfo::CreateStruct() const
{
    ASSERT( IsStruct() );
    ASSERT( !IsAbstract() );
    return (Struct *)Create();
}

// SetArraySize
//------------------------------------------------------------------------------
void ReflectionInfo::SetArraySize( void * array, size_t size ) const
{
    ASSERT( IsStruct() );
    ASSERT( !IsAbstract() );
    SetArraySizeV( array, size );
}

// Create
//------------------------------------------------------------------------------
/*virtual*/ void * ReflectionInfo::Create() const
{
    ASSERT( false ); // Should be implemented by derived class!
    return nullptr;
}

// SetArraySizeV
//------------------------------------------------------------------------------
/*virtual*/ void ReflectionInfo::SetArraySizeV( void * UNUSED( array ), size_t UNUSED( size ) ) const
{
    ASSERT( false ); // Should be implemented by derived class!
}

// Load
//------------------------------------------------------------------------------
/*static*/ Object * ReflectionInfo::Load( const char * scopedName )
{
    AStackString<> fullPath;
    fullPath.Format( "Reflection\\%s.obj", scopedName );

    FileStream fs;
    if ( fs.Open( fullPath.Get(), FileStream::READ_ONLY ) == false )
    {
        return nullptr;
    }

    const size_t fileSize = (size_t)fs.GetFileSize();
    AutoPtr< char > mem( (char *)Alloc( fileSize + 1 ) );
    if ( fs.Read( mem.Get(), fileSize ) != fileSize )
    {
        return nullptr;
    }
    mem.Get()[ fileSize ] = 0;

    ConstMemoryStream ms( mem.Get(), fileSize + 1 );
    TextReader tr( ms );
    RefObject * refObject = tr.Read();
    ASSERT( !refObject || DynamicCast< Object >( refObject ) );

    return (Object *)refObject;
}

// RegisterRootObject
//------------------------------------------------------------------------------
/*static*/ void ReflectionInfo::RegisterRootObject( Object * obj )
{
    s_RootObjects.Append( obj );
}

// FindObjectByScopedName
//------------------------------------------------------------------------------
/*static*/ Object * ReflectionInfo::FindObjectByScopedName( const AString & scopedName )
{
    // split into individual names
    Array< AString > names;
    scopedName.Tokenize( names, '.' );
    const size_t size = names.GetSize();
    ASSERT( size );

    const Array< Object * > * children = &s_RootObjects;

    size_t depth = 0;

    for (;;)
    {
        // find object at this scope
        Object * currentObj = nullptr;
        const AString & lookingFor = names[ depth ];
        const size_t numChildren = children->GetSize();
        for ( size_t i=0; i<numChildren; ++i )
        {
            Object * thisChild = ( *children )[ i ];
            if ( thisChild->GetName() == lookingFor )
            {
                currentObj = thisChild;
                break;
            }
        }

        if ( currentObj == nullptr )
        {
            return nullptr; // not found
        }

        // are we at the final object?
        if ( depth == ( size - 1 ) )
        {
            return currentObj; // success
        }

        // is this a container?
        Container * currentContainer = DynamicCast< Container >( currentObj );
        if ( currentContainer == nullptr )
        {
            return nullptr; // no more child objects
        }

        children = &currentContainer->GetChildren();
        depth++;
    }
}

// WriteDefinitions
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    /*static*/ bool ReflectionInfo::WriteDefinitions()
    {
        uint32_t numProblems = 0;

        const ReflectionInfo * ri = s_FirstReflectionInfo;
        for ( ; ri != nullptr; ri = ri->m_Next )
        {
            // ignore abstract classes
            if ( ri->IsAbstract() )
            {
                continue;
            }

            // Serialize a default instance to a MemoryStream
            MemoryStream ms;
            {
                // Create and serialize default instance
                if ( ri->IsObject() )
                {
                    RefObject * object = ri->CreateObject();
                    {
                        TextWriter tw( ms );
                        tw.Write( object );
                    }
                    FDELETE( object );
                }
                else
                {
                    ASSERT( ri->IsStruct() );
                    Struct * str = ri->CreateStruct();
                    {
                        TextWriter tw( ms );
                        tw.Write( str, ri );
                    }
                    FDELETE( str );
                }
            }

            AStackString<> fileName;
            fileName.Format( "..\\Data\\Reflection\\.Definitions\\%s.definition", ri->GetTypeName() );

            // avoid writing file if not changed

            // Try to open existing file
            FileStream f;
            if ( f.Open( fileName.Get(), FileStream::READ_ONLY ) )
            {
                // read content
                const uint64_t fileSize = f.GetFileSize();
                if ( fileSize == ms.GetSize() )
                {
                    AutoPtr< char > mem( (char *)Alloc( (size_t)fileSize ) );
                    if ( f.Read( mem.Get(), (size_t)fileSize ) == fileSize )
                    {
                        if ( memcmp( mem.Get(), ms.GetData(), (size_t)fileSize ) == 0 )
                        {
                            continue; // definition has not changed
                        }
                    }
                }
                f.Close();
            }

            // Definition changed - try to save it

            int result = 0;
            AutoPtr< char > memOut;
            AutoPtr< char > memErr;
            uint32_t memOutSize;
            uint32_t memErrSize;

            // existing definition?
            if ( FileIO::FileExists( fileName.Get() ) )
            {
                // existing - need to open for edit?
                if ( FileIO::GetReadOnly( fileName ) )
                {
                    AStackString<> args( "edit " );
                    args += fileName;
                    Process p;
                    if ( p.Spawn( "p4", args.Get(), nullptr, nullptr ) )
                    {
                        p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );
                        result = p.WaitForExit();
                    }
                }
            }
            else
            {
                // new - open for add
                AStackString<> args( "add " );
                args += fileName;
                Process p;
                if ( p.Spawn( "p4", args.Get(), nullptr, nullptr ) )
                {
                    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );
                    result = p.WaitForExit();
                }
            }

            if ( result == 0 )
            {
                if ( f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
                {
                    if ( f.Write( ms.GetData(), ms.GetSize() ) == ms.GetSize() )
                    {
                        continue; // all ok!
                    }
                }
            }

            // PROBLEM!
            OUTPUT( "Error writing definition '%s'\n", fileName.Get() );
            if ( result != 0 )
            {
                OUTPUT( "Perforce error: %s\n", memErr.Get() );
            }
            ++numProblems;
        }
        if ( numProblems > 0 )
        {
            FATALERROR( "Problem writing %u definition(s).\n", numProblems );
        }
        return ( numProblems == 0 );
    }
#endif

//------------------------------------------------------------------------------
