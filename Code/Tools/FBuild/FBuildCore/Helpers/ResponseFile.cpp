// ResponseFile
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ResponseFile.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ResponseFile::ResponseFile()
    : m_EscapeSlashes( false )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ResponseFile::~ResponseFile()
{
    if ( m_ResponseFilePath.IsEmpty() == false )
    {
        FileIO::FileDelete( m_ResponseFilePath.Get() );
    }
}

// Create
//------------------------------------------------------------------------------
bool ResponseFile::Create( const Args & args )
{
    return Create( args.GetFinalArgs() );
}

// Create
//------------------------------------------------------------------------------
bool ResponseFile::Create( const AString & contents )
{
    if ( m_EscapeSlashes )
    {
        AStackString< 1024 > fixed;
        if ( contents.GetLength() > 512 )
        {
            fixed.SetReserved( contents.GetLength() * 2 );
        }
        const char * it = contents.Get();
        const char * end = contents.GetEnd();
        char * dst = fixed.Get();
        while ( it != end )
        {
            char c = *it;
            if ( ( c == BACK_SLASH ) || ( c == FORWARD_SLASH ) )
            {
                *dst = BACK_SLASH; dst++;
                *dst = BACK_SLASH; dst++;
            }
            else
            {
                *dst = c; dst++;
            }
            it++;
        }
        fixed.SetLength( (uint32_t)( dst - fixed.Get() ) );

        return CreateInternal( fixed );
    }

    return CreateInternal( contents );
}

// CreateInternal
//------------------------------------------------------------------------------
bool ResponseFile::CreateInternal( const AString & contents )
{
    // store in tmp folder, and give back to user
    WorkerThread::CreateTempFilePath( "args.rsp", m_ResponseFilePath );

    // write file to disk
    const uint32_t flags = FileStream::WRITE_ONLY       // we only want to write
                         | FileStream::TEMP;            // avoid flush to disk if possible
    if ( !m_File.Open( m_ResponseFilePath.Get(), flags ) )
    {
        FileIO::WorkAroundForWindowsFilePermissionProblem( m_ResponseFilePath, flags, 5 ); // 5s max wait

        // Retry
        if ( !m_File.Open( m_ResponseFilePath.Get(), flags ) )
        {
            FLOG_ERROR( "Failed to create response file '%s'", m_ResponseFilePath.Get() );
            return false; // user must handle error
        }
    }

    bool ok = ( m_File.Write( contents.Get(), contents.GetLength() ) == contents.GetLength() );
    if ( !ok )
    {
        FLOG_ERROR( "Failed to write response file '%s'", m_ResponseFilePath.Get() );
    }

    m_File.Close(); // must be closed so MSVC link.exe can open it

    FileIO::WorkAroundForWindowsFilePermissionProblem( m_ResponseFilePath );

    return ok;
}

//------------------------------------------------------------------------------
