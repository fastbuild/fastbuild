// BFFFileExists
//
// Track #if file_exists checks
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class IOStream;

// BFFFileExists
//------------------------------------------------------------------------------
class BFFFileExists
{
public:
    explicit BFFFileExists();
    ~BFFFileExists();

    // When parsing, file existing is checked/tracked and saved to the DB
    bool CheckFile( const AString & fileName );
    void Save( IOStream & stream ) const;

    // When loading an existing DB, we check if anything changed
    bool Load( IOStream & stream );
    const AString * CheckForChanges( bool & outAdded ) const;

private:
    Array< AString >    m_FileNames;
    Array< bool >       m_FileExists;
};

//------------------------------------------------------------------------------
