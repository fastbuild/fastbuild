// BFFFileExists
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFFileExists.h"

// Core
#include "Core/FileIO/FileIO.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFFileExists::BFFFileExists() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFFileExists::~BFFFileExists() = default;

// CheckFile
//------------------------------------------------------------------------------
bool BFFFileExists::CheckFile( const AString & fileName )
{
    // Did we check for this file already?
    const AString * found = m_FileNames.Find( fileName );
    if ( found )
    {
        // Yes - return existing result
        const size_t index = (size_t)( found - m_FileNames.Begin() );
        return m_FileExists[ index ];
    }

    // Checking for first time
    const bool exists = FileIO::FileExists( fileName.Get() );

    // Record dependency and result
    m_FileNames.Append( fileName );
    m_FileExists.Append( exists );

    return exists;
}

// Save
//------------------------------------------------------------------------------
void BFFFileExists::Save( IOStream & stream ) const
{
    stream.Write( m_FileNames );
    stream.Write( m_FileExists );
}

// Load
//------------------------------------------------------------------------------
bool BFFFileExists::Load( IOStream & stream )
{
    ASSERT( m_FileNames.IsEmpty() ); // Must only be called on empty object

    return ( stream.Read( m_FileNames ) && stream.Read( m_FileExists ) );
}

// CheckForChanges
//------------------------------------------------------------------------------
const AString * BFFFileExists::CheckForChanges( bool & outAdded ) const
{
    for ( size_t i = 0; i < m_FileNames.GetSize(); ++i )
    {
        const bool exists = FileIO::FileExists( m_FileNames[ i ].Get() );
        if ( exists != m_FileExists[ i ] )
        {
            outAdded = exists;
            return &m_FileNames[ i ];
        }
    }

    // No files changed
    return nullptr;
}

//------------------------------------------------------------------------------
