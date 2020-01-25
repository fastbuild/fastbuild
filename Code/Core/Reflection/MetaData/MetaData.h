// MetaData.h
//------------------------------------------------------------------------------
#pragma once

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
class MetaNone {};

// Basic MetaData Types
//------------------------------------------------------------------------------
IMetaData & MetaFile( bool relative = false );
IMetaData & MetaHidden();
IMetaData & MetaOptional();
IMetaData & MetaPath( bool relative = false );
IMetaData & MetaRange( int32_t minVal, int32_t maxVal );

//------------------------------------------------------------------------------
