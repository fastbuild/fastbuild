// MetaData.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_METADATA_H
#define CORE_REFLECTION_METADATA_H

// Forward Declarations
//------------------------------------------------------------------------------
class IMetaData;

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Chaining operator for reflection macros
//------------------------------------------------------------------------------
IMetaData & operator + ( IMetaData & a, IMetaData & b );

// No MetaData
//------------------------------------------------------------------------------
IMetaData & MetaNone();

// Basic MetaData Types
//------------------------------------------------------------------------------
IMetaData & MetaFile( bool relative = false );
IMetaData & MetaOptional();
IMetaData & MetaPath( bool relative = false );
IMetaData & MetaRange( uint32_t minVal, uint32_t maxVal );

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_METADATA_H
