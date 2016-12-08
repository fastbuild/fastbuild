// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildWorkerOptions.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// FBuildWorkerOptions (CONSTRUCTOR)
//------------------------------------------------------------------------------
FBuildWorkerOptions::FBuildWorkerOptions() 
	: m_OverrideCPUAllocationWhenIdle( false )
    , m_CPUAllocationWhenIdle( 0 )
	, m_OverrideCPUAllocationDedicated(false)
	, m_CPUAllocationDedicated(0)
#if defined( __WINDOWS__ )
	, m_IsSubprocess(false)
	, m_UseSubprocess(true)
#endif
{
}

// TokenOffset should be the length of the first portion of the string. For example, for a token of "-gpu=4", the tokenOffset should be strlen("-gpu=")
bool ParseCpuCommandLine(const AString& token, size_t tokenOffset, bool& overrideCpuAllocation, uint32_t& cpuAllocation)
{
	int32_t numCPUs = Env::GetNumProcessors();
	int32_t num(0);
	if (sscanf(token.Get() + tokenOffset, "%i", &num) == 1)
	{
		if (token.EndsWith('%'))
		{
			num = (int32_t)(numCPUs * (float)num / 100.0f);
			cpuAllocation = Math::Clamp(num, 1, numCPUs);
			overrideCpuAllocation = true;
			return true;
		}
		else if (num > 0)
		{
			cpuAllocation = Math::Clamp(num, 1, numCPUs);
			overrideCpuAllocation = true;
			return true;
		}
		else if (num < 0)
		{
			cpuAllocation = Math::Clamp((numCPUs + num), 1, numCPUs);
			overrideCpuAllocation = true;
			return true;
		}
		// problem... fall through
	}
	// problem... fall through
	return false;
}

// ProcessCommandLine
//------------------------------------------------------------------------------
bool FBuildWorkerOptions::ProcessCommandLine( const AString & commandLine )
{
    // Tokenize
    Array< AString > tokens;
    commandLine.Tokenize( tokens );

	// Keep backwards compatibility for CPU selection and disabling work.
	// Since the legacy settings are applied at the end, they have the opportunity to stomp the new cpusWhenIdle, cpusDedicated settings.
	uint32_t legacyCPUAllocation = 0;
	bool legacyOverrideCPUAllocation = false;
	bool legacyWorkModeDisabled = false;
	bool legacyWorkModeIdle = false;
	bool legacyWorkModeDedicated = false;
	bool legacyOverrideWorkMode = false;

    // Check each token
    const AString * const end = tokens.End();
    for ( const AString * it=tokens.Begin(); it != end; ++it )
    {
        const AString & token = *it;
		if (token.BeginsWith("-cpus="))
		{
			int32_t numCPUs = Env::GetNumProcessors();
			int32_t num(0);
			if (sscanf(token.Get() + 6, "%i", &num) == 1)
			{
				if (token.EndsWith('%'))
				{
					num = (int32_t)(numCPUs * (float)num / 100.0f);
					legacyCPUAllocation = Math::Clamp(num, 1, numCPUs);
					legacyOverrideCPUAllocation = true;
					continue;
				}
				else if (num > 0)
				{
					legacyCPUAllocation = Math::Clamp(num, 1, numCPUs);
					legacyOverrideCPUAllocation = true;
					continue;
				}
				else if (num < 0)
				{
					legacyCPUAllocation = Math::Clamp((numCPUs + num), 1, numCPUs);
					legacyOverrideCPUAllocation = true;
					continue;
				}
				// problem... fall through
			}
			// problem... fall through
		}
		else if (token == "-mode=disabled")
		{
			legacyWorkModeDisabled = true;
			legacyWorkModeIdle = false;
			legacyWorkModeDedicated = false;
			legacyOverrideWorkMode = true;
			continue;
		}
		else if (token == "-mode=idle")
		{
			legacyWorkModeDisabled = false;
			legacyWorkModeIdle = true;
			legacyWorkModeDedicated = false;
			legacyOverrideWorkMode = true;
			continue;
		}
		else if (token == "-mode=dedicated")
		{
			legacyWorkModeDisabled = false;
			legacyWorkModeIdle = false;
			legacyWorkModeDedicated = true;
			legacyOverrideWorkMode = true;
			continue;
		}


        if ( token.BeginsWith( "-cpusWhenIdle=" ) )
        {
			if (ParseCpuCommandLine(token, strlen("-cpusWhenIdle="), m_OverrideCPUAllocationWhenIdle, m_CPUAllocationWhenIdle))
			{
				continue;
			}
        }
		if (token.BeginsWith("-cpusDedicated="))
		{
			if (ParseCpuCommandLine(token, strlen("-cpusDedicated="), m_OverrideCPUAllocationDedicated, m_CPUAllocationDedicated))
			{
				continue;
			}
		}
        #if defined( __WINDOWS__ )
            else if ( token == "-nosubprocess" )
            {
                m_UseSubprocess = false;
                continue;
            }
            else if ( token == "-subprocess" ) // Internal option only!
            {
                m_IsSubprocess = true;
                continue;
            }
        #endif

        ShowUsageError();
        return false;
    }

	if (legacyOverrideWorkMode && legacyWorkModeDisabled)
	{
		m_OverrideCPUAllocationWhenIdle = true;
		m_CPUAllocationWhenIdle = 0;
		m_OverrideCPUAllocationDedicated = true;
		m_CPUAllocationDedicated = 0;
	}
	else if (legacyOverrideWorkMode && legacyWorkModeIdle && legacyOverrideCPUAllocation)
	{
		m_OverrideCPUAllocationWhenIdle = true;
		m_CPUAllocationWhenIdle = legacyCPUAllocation;
	}
	else if (legacyOverrideWorkMode && legacyWorkModeDedicated && legacyOverrideCPUAllocation)
	{
		m_OverrideCPUAllocationDedicated = true;
		m_CPUAllocationDedicated = legacyCPUAllocation;
	}

    return true;
}

// ShowUsageError
//------------------------------------------------------------------------------
void FBuildWorkerOptions::ShowUsageError()
{
    const char * msg = "FBuildWorker.exe - " FBUILD_VERSION_STRING " (" FBUILD_VERSION_PLATFORM ")\n"
                       "\n"
                       "Command Line Options:\n"
                       "------------------------------------------------------------\n"
                       "-cpusWhenIdle=[n|-n|n%] : Set number of CPUs to use when idle.\n"
                       "                n : Explicit number.\n"
                       "                -n : NUMBER_OF_PROCESSORS-n.\n"
                       "                n% : % of NUMBER_OF_PROCESSORS.\n"
                       "\n"
						"-cpusDedicated=[n|-n|n%] : Set number of dedicated CPUs to use.\n"
						"                n : Explicit number.\n"
						"                -n : NUMBER_OF_PROCESSORS-n.\n"
						"                n% : % of NUMBER_OF_PROCESSORS.\n"
						"\n"
                       #if defined( __WINDOWS__ )
                       "-nosubprocess : Don't spawn a sub-process worker copy.\n";
                       #else
                       ;
                       #endif

    #if defined( __WINDOWS__ )
        ::MessageBox( nullptr, msg, "FBuildWorker - Bad Command Line", MB_ICONERROR | MB_OK );
    #else
        printf( "%s", msg );
        (void)msg; // TODO:MAC Fix missing MessageBox
        (void)msg; // TODO:LINUX Fix missing MessageBox
    #endif
}

//------------------------------------------------------------------------------
