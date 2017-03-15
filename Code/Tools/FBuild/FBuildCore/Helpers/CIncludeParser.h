// CIncludeParser - Parse a c, cpp or h file and return all included headers
//                  recursively.
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// CIncludeParser class
//------------------------------------------------------------------------------
class CIncludeParser
{
public:
    explicit CIncludeParser();
    ~CIncludeParser();

    bool ParseMSCL_Output( const char * compilerOutput, size_t compilerOutputSize );
    bool ParseMSCL_Preprocessed( const char * compilerOutput, size_t compilerOutputSize );
    bool ParseGCC_Preprocessed( const char * compilerOutput, size_t compilerOutputSize );

    const Array< AString > & GetIncludes() const { return m_Includes; }

    // take ownership of includes array to avoid re-allocations
    void SwapIncludes( Array< AString > & includes );
    #ifdef DEBUG
        inline size_t GetNonUniqueCount() const { return m_NonUniqueCount; }
    #endif

private:
    static void ParseToNextLineStartingWithHash( const char * & pos );

    void AddInclude( const char * begin, const char * end );

    // temporary data
    uint32_t            m_LastCRC1;
    Array< uint32_t >   m_CRCs1;
    uint32_t            m_LastCRC2;
    Array< uint32_t >   m_CRCs2;

    // final data
    Array< AString > m_Includes;    // list of unique includes
    #ifdef DEBUG
        size_t m_NonUniqueCount;    // number of include directives seen
    #endif
};

//------------------------------------------------------------------------------
