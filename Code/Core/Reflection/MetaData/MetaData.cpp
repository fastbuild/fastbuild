// MetaData
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "MetaData.h"
#include "Core/Mem/Mem.h"
#include "Core/Reflection/MetaData/Meta_File.h"
#include "Core/Reflection/MetaData/Meta_Optional.h"
#include "Core/Reflection/MetaData/Meta_Path.h"
#include "Core/Reflection/MetaData/Meta_Range.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_BEGIN( IMetaData, RefObject, MetaNone() )
REFLECT_END( IMetaData )

// CONSTRUCTOR
//------------------------------------------------------------------------------
IMetaData::IMetaData()
	: m_Next( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
IMetaData::~IMetaData()
{
}


// Chaining operator for reflection macros
//------------------------------------------------------------------------------
IMetaData & operator + ( IMetaData & a, IMetaData & b )
{
	a.m_Next = &b;
	return a;
}

// operator +
//------------------------------------------------------------------------------
/*IMetaData & IMetaData::operator + ( IMetaData & metaData )
{
	m_Next = &metaData;
	return *this;
}*/

// No MetaData
//------------------------------------------------------------------------------
IMetaData & MetaNone()
{
	// We have to return by reference to be able to implement the chainign + operator
	// but everything is managed as a ptr internally so this is ok
	IMetaData * md = nullptr;
	PRAGMA_DISABLE_PUSH_MSVC( 6011 ); // null deref is deliberate
	IMetaData& mdRef = *md;
	PRAGMA_DISABLE_POP_MSVC
	return mdRef;
}

// Basic MetaData Types
//------------------------------------------------------------------------------

// MetaFile
//------------------------------------------------------------------------------
IMetaData & MetaFile( bool relative )
{
	return *FNEW( Meta_File( relative ) );
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
IMetaData & MetaRange( uint32_t minVal, uint32_t maxVal )
{
	return *FNEW( Meta_Range( minVal, maxVal ) );
}

//------------------------------------------------------------------------------
