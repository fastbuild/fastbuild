// MetaData
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MetaData.h"
#include "Core/Mem/Mem.h"
#include "Core/Reflection/MetaData/Meta_File.h"
#include "Core/Reflection/MetaData/Meta_Hidden.h"
#include "Core/Reflection/MetaData/Meta_Optional.h"
#include "Core/Reflection/MetaData/Meta_Path.h"
#include "Core/Reflection/MetaData/Meta_Range.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_BEGIN( IMetaData, Object, MetaNone() )
REFLECT_END( IMetaData )

// CONSTRUCTOR
//------------------------------------------------------------------------------
IMetaData::IMetaData()
    : m_Next( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
IMetaData::~IMetaData() = default;

// Chaining operator for reflection macros
//------------------------------------------------------------------------------
IMetaData & operator + ( IMetaData & a, IMetaData & b )
{
    b.m_Next = a.m_Next;
    a.m_Next = &b;
    return a;
}

// Basic MetaData Types
//------------------------------------------------------------------------------

// MetaFile
//------------------------------------------------------------------------------
IMetaData & MetaFile( bool relative )
{
    return *FNEW( Meta_File( relative ) );
}

// MetaHidden
//------------------------------------------------------------------------------
IMetaData & MetaHidden()
{
    return *FNEW( Meta_Hidden() );
}

// MetaOptional
//------------------------------------------------------------------------------
IMetaData & MetaOptional()
{
    return *FNEW( Meta_Optional() );
}

// MetaPath
//------------------------------------------------------------------------------
IMetaData & MetaPath( bool relative )
{
    return *FNEW( Meta_Path( relative ) );
}

// MetaRange
//------------------------------------------------------------------------------
IMetaData & MetaRange( int32_t minVal, int32_t maxVal )
{
    return *FNEW( Meta_Range( minVal, maxVal ) );
}

//------------------------------------------------------------------------------
