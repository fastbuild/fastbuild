// BFFFile
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFFile.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Error.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/xxHash.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFFile::BFFFile() = default;

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFFile::BFFFile( const char * fileName, const AString & fileContents )
    : m_FileName( fileName )
    , m_FileContents( fileContents )
    , m_Once( false )
{
    m_Hash = xxHash::Calc64( m_FileContents );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFFile::~BFFFile() = default;

// Load
//------------------------------------------------------------------------------
bool BFFFile::Load( const AString & fileName, const BFFToken * token )
{
    FLOG_VERBOSE( "Loading BFF '%s'", fileName.Get() );

    // Open the file
    FileStream bffStream;
    if ( bffStream.Open( fileName.Get() ) == false )
    {
        // missing bff is a fatal problem
        if ( token )
        {
            Error::Error_1032_UnableToOpenInclude( token, fileName );
        }
        else
        {
            FLOG_ERROR( "Failed to open BFF '%s'", fileName.Get() );
        }
        return false;
    }

    // read entire config into memory
    const uint32_t size = (uint32_t)bffStream.GetFileSize();
    AString fileContents;
    fileContents.SetLength( size );
    if ( bffStream.Read( fileContents.Get(), size ) != size )
    {
        FLOG_ERROR( "Error reading BFF '%s'", fileName.Get() );
        return false;
    }

    // Store details
    m_FileContents = Move( fileContents );
    m_FileName = fileName;
    m_ModTime = FileIO::GetFileLastWriteTime( fileName );
    m_Hash = xxHash::Calc64( m_FileContents );

    return true;
}

//------------------------------------------------------------------------------
