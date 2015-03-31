// FBuild - the main application
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuildCore
#include "FBuildOptions.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"

// CONSTRUCTOR - FBuildOptions
//------------------------------------------------------------------------------
FBuildOptions::FBuildOptions()
: m_ForceCleanBuild( false )
, m_UseCacheRead( false )
, m_UseCacheWrite( false )
, m_ShowInfo( false )
, m_ShowCommandLines( false )
, m_ShowErrors( true )
, m_ShowProgress( false )
, m_AllowDistributed( false )
, m_ShowSummary( false )
, m_ShowBuildTime( true )
, m_SaveDBOnCompletion( false )
, m_GenerateReport( false )
, m_NoLocalConsumptionOfRemoteJobs( false )
, m_AllowLocalRace( true )
, m_WrapperChild( false )
, m_FixupErrorPaths( false )
, m_StopOnFirstError( true )
{
#ifdef DEBUG
	//m_ShowInfo = true; // uncomment this to enable spam when debugging
#endif

	// Default to NUMBER_OF_PROCESSORS
	m_NumWorkerThreads = Env::GetNumProcessors();
}

// FBuildOptions::SetWorkingDir
//------------------------------------------------------------------------------
void FBuildOptions::SetWorkingDir( const AString & path )
{
	m_WorkingDir = path;
	if ( m_WorkingDir.IsEmpty() )
	{
		return;
	}

	// clean path
	PathUtils::FixupFolderPath( m_WorkingDir );
        
    // no trailing slash
    if ( m_WorkingDir.EndsWith( NATIVE_SLASH ) )
    {
        m_WorkingDir.SetLength( m_WorkingDir.GetLength() - 1 );
    }

	#if defined( __WINDOWS__ )
		// so C:\ and c:\ are treated the same on Windows, for better cache hits
		// make the drive letter always uppercase
		if ( ( m_WorkingDir.GetLength() >= 2 ) &&
			 ( m_WorkingDir[ 1 ] == ':' ) &&
			 ( m_WorkingDir[ 0 ] >= 'a' ) &&
			 ( m_WorkingDir[ 0 ] <= 'z' ) )
		{
			m_WorkingDir[ 0 ] = ( 'A' + ( m_WorkingDir[ 0 ] - 'a' ) );
		}
	#endif
}

AString FBuildOptions::GetMainProcessMutexName() const
{
    AString result;
    if(m_WorkingDir.IsEmpty())
    {
        result = "Global\\FASTBuild";
    }
    else
    {
        uint32_t workingDirCRC = CRC32::CalcLower(m_WorkingDir);
        result.Format("Global\\FASTBuild-0x%08x", workingDirCRC);
    }
    return result;
}

AString FBuildOptions::GetFinalProcessMutexName() const
{
    AString result;
    if (m_WorkingDir.IsEmpty())
    {
        result = "Global\\FASTBuild_Final";
    }
    else
    {
        uint32_t workingDirCRC = CRC32::CalcLower(m_WorkingDir);
        result.Format("Global\\FASTBuild_Final-0x%08x", workingDirCRC);
    }
    return result;
}

AString FBuildOptions::GetSharedMemoryName() const
{
    AString result;
    if (m_WorkingDir.IsEmpty())
    {
        result = "FASTBuildSharedMemory";
    }
    else
    {
        uint32_t workingDirCRC = CRC32::CalcLower(m_WorkingDir);
        result.Format("FASTBuildSharedMemory_%08x", workingDirCRC);
    }
    return result;
}

//------------------------------------------------------------------------------
