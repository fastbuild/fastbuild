// BFFFile
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFToken;

// BFFFile
//------------------------------------------------------------------------------
class BFFFile
{
public:
    BFFFile();
    BFFFile( const char * fileName, const AString & fileContents );
    ~BFFFile();

    bool Load( const AString & fileName, const BFFToken * token );

    const AString & GetFileName() const             { return m_FileName; }
    const AString & GetSourceFileContents() const   { return m_FileContents; }
    bool            IsParseOnce() const             { return m_Once; }
    uint64_t        GetTimeStamp() const            { return m_ModTime; }
    uint64_t        GetHash() const                 { return m_Hash; }

    // Set during tokenization
    void            SetParseOnce() const { m_Once = true; }

protected:
    AString         m_FileName;
    AString         m_FileContents;
    mutable bool    m_Once          = false; // Set if #once directive is seen
    uint64_t        m_ModTime       = 0;
    uint64_t        m_Hash          = 0;
};

//------------------------------------------------------------------------------
