// ResponseFile
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class AString;

// ResponseFile
//------------------------------------------------------------------------------
class ResponseFile
{
public:
    explicit ResponseFile();
    ~ResponseFile();

    bool Create( const Args & args );
    bool Create( const AString & contents );
    const AString & GetResponseFilePath() const { return m_ResponseFilePath; }

    void SetEscapeSlashes() { m_EscapeSlashes = true; }
private:
    bool CreateInternal( const AString & contents );

    FileStream m_File;
    AStackString<> m_ResponseFilePath;
    bool m_EscapeSlashes;
};

//------------------------------------------------------------------------------
