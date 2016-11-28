// CRC32.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// CRC32
//------------------------------------------------------------------------------
class CRC32
{
public:
    static inline uint32_t  Start() { return 0xFFFFFFFF; }
    static uint32_t         Update( uint32_t crc32, const void * buffer, size_t len );
    static uint32_t         UpdateLower( uint32_t crc32, const void * buffer, size_t len );
    static inline uint32_t  Stop( uint32_t crc32 ) { return ( crc32 ^ 0xFFFFFFFF ); }

    static uint32_t         Calc( const void * buffer, size_t len );
    static uint32_t         CalcLower( const void * buffer, size_t len );

    inline static uint32_t  Calc( const AString & string )      { return Calc( string.Get(), string.GetLength() ); }
    inline static uint32_t  CalcLower( const AString & string ) { return CalcLower( string.Get(), string.GetLength() ); }
};

//------------------------------------------------------------------------------
