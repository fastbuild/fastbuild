// MultiBuffer
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MultiBuffer.h"

// Core
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Strings/AString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
MultiBuffer::MultiBuffer()
    : m_ReadStream( nullptr )
    , m_WriteStream( nullptr )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
MultiBuffer::MultiBuffer( const void * data, size_t dataSize )
    : m_ReadStream( FNEW( ConstMemoryStream( data, dataSize ) ) )
    , m_WriteStream( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
MultiBuffer::~MultiBuffer()
{
    FDELETE( m_ReadStream );
    FDELETE( m_WriteStream );
}

// SerializeFromFiles
//------------------------------------------------------------------------------
bool MultiBuffer::CreateFromFiles( const Array< AString > & fileNames, size_t * outproblemFileIndex )
{
    ASSERT( fileNames.GetSize() <= MAX_FILES );
    ASSERT( ( m_ReadStream == nullptr ) && ( m_WriteStream == nullptr ) );

    uint64_t fileSizes[ MAX_FILES ];
    FileStream fileStreams[ MAX_FILES ];
    const size_t numFiles = fileNames.GetSize();

    // Open all the files and determine their size
    uint64_t memSize = sizeof( uint32_t ); // write number of files
    for ( size_t i = 0; i <numFiles; ++i )
    {
        FileStream& fs = fileStreams[ i ];
        if ( fs.Open( fileNames[ i ].Get(), FileStream::READ_ONLY ) == false )
        {
            if ( outproblemFileIndex )
            {
                *outproblemFileIndex = i;
            }
            return false;
        }
        const uint64_t fileSize = fs.GetFileSize();
        memSize += ( sizeof( uint64_t ) + fileSize );
        fileSizes[ i ] = fileSize;
    }

    // Allocate enough space for the concatenated output
    m_WriteStream = FNEW( MemoryStream( (size_t)memSize ) );

    // Write number of files
    m_WriteStream->Write( (uint32_t)numFiles );

    // Write size of each file
    for ( size_t i = 0; i <numFiles; ++i )
    {
        m_WriteStream->Write( (uint64_t)fileSizes[ i ] );
    }

    // Read data for each file
    for ( size_t i = 0; i <numFiles; ++i )
    {
        FileStream & fs = fileStreams[ i ];
        if ( m_WriteStream->WriteBuffer( fs, fileSizes[ i ] ) != fileSizes[ i ] )
        {
            if ( outproblemFileIndex )
            {
                *outproblemFileIndex = i;
            }
            return false;
        }
    }

    // Check we wrote as much as we originaly calculated
    ASSERT( m_WriteStream->GetSize() == memSize );
    return true;
}

// ExtractFile
//------------------------------------------------------------------------------
bool MultiBuffer::ExtractFile( size_t index, const AString& fileName ) const
{
    ASSERT( m_ReadStream );

    m_ReadStream->Seek( 0 );
    uint32_t numFiles;
    m_ReadStream->Read( numFiles );

    ASSERT( index <= numFiles );

    // work out data offset from file sizes
    uint64_t offset = sizeof( uint32_t ) + ( sizeof( uint64_t ) * numFiles );
    for ( size_t i=0; i<index; ++i )
    {
        uint64_t fileSize;
        m_ReadStream->Read( fileSize );
        offset += fileSize;
    }

    // size of the file we want to write
    uint64_t fileSize = 0;
    m_ReadStream->Read( fileSize );

    // Jump to data
    m_ReadStream->Seek( offset );
    const void * fileData = (void *)( (size_t)m_ReadStream->GetData() + offset );

    FileStream fs;
    if ( !fs.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        // On Windows, we can occasionally fail to open the file with error 1224 (ERROR_USER_MAPPED_FILE), due to
        // things like anti-virus etc. Simply retry if that happens
        // Also, when a <LOCAL RACE> occurs, the local compilation process might not have exited at this point
        // (we call ::TerminateProcess, which is async),which can cause failure below, because the file is still locked.
        FileIO::WorkAroundForWindowsFilePermissionProblem( fileName, FileStream::WRITE_ONLY, 15 ); // 15 secs max wait

        // Try again
        if ( !fs.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
        {
            return false;
        }
    }
    if ( fs.WriteBuffer( fileData, fileSize ) != fileSize )
    {
        return false;
    }

    return true;
}

// GetData
//------------------------------------------------------------------------------
const void * MultiBuffer::GetData() const
{
    return m_ReadStream ? m_ReadStream->GetData() : m_WriteStream->GetData();
}

// GetDataSize
//------------------------------------------------------------------------------
uint64_t MultiBuffer::GetDataSize() const
{
    return m_ReadStream ? m_ReadStream->GetSize() : m_WriteStream->GetSize();
}

// Release
//------------------------------------------------------------------------------
void * MultiBuffer::Release( size_t & outSize )
{
    ASSERT( m_WriteStream ); // Only supported on a MultiBuffer we built
    outSize = m_WriteStream->GetSize();
    return m_WriteStream->Release();
}

//------------------------------------------------------------------------------
