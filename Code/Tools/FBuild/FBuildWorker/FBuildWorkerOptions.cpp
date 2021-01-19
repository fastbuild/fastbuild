// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildWorkerOptions.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// FBuildWorkerOptions (CONSTRUCTOR)
//------------------------------------------------------------------------------
FBuildWorkerOptions::FBuildWorkerOptions() :
#if defined( __WINDOWS__ )
    m_IsSubprocess( false ),
    m_UseSubprocess( true ),
#endif
    m_OverrideCPUAllocation( false ),
    m_CPUAllocation( 0 ),
    m_OverrideWorkMode( false ),
    m_WorkMode( WorkerSettings::WHEN_IDLE ),
    m_MinimumFreeMemoryMiB( 0 ),
    m_ConsoleMode( false )
{
    #ifdef __LINUX__
        m_ConsoleMode = true; // Only console mode supported on Linux
    #endif
}

// ProcessCommandLine
//------------------------------------------------------------------------------
bool FBuildWorkerOptions::ProcessCommandLine( const AString & commandLine )
{
    // Tokenize
    Array< AString > tokens;
    commandLine.Tokenize( tokens );

    // Check each token
    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it != end; ++it )
    {
        const AString & token = *it;
        #if defined( __WINDOWS__ ) || defined( __OSX__ )
            if ( token == "-console" )
            {
                m_ConsoleMode = true;
                #if defined( __WINDOWS__ )
                    m_UseSubprocess = false;
                #endif
                continue;
            }
        #endif
        if ( token.BeginsWith( "-cpus=" ) )
        {
            int32_t numCPUs = (int32_t)Env::GetNumProcessors();
            int32_t num( 0 );
            PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
            PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
            if ( sscanf( token.Get() + 6, "%i", &num ) == 1 ) // TODO:C consider sscanf_s
            PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
            PRAGMA_DISABLE_POP_MSVC // 4996
            {
                if ( token.EndsWith( '%' ) )
                {
                    num = (int32_t)( (float)numCPUs * (float)num / 100.0f );
                    m_CPUAllocation = (uint32_t)Math::Clamp( num, 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                else if ( num > 0 )
                {
                    m_CPUAllocation = (uint32_t)Math::Clamp( num, 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                else if ( num < 0 )
                {
                    m_CPUAllocation = (uint32_t)Math::Clamp( ( numCPUs + num ), 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                // problem... fall through
            }
            // problem... fall through
        }
        else if ( token == "-mode=disabled" )
        {
            m_WorkMode = WorkerSettings::DISABLED;
            m_OverrideWorkMode = true;
            continue;
        }
        else if ( token == "-mode=idle" )
        {
            m_WorkMode = WorkerSettings::WHEN_IDLE;
            m_OverrideWorkMode = true;
            continue;
        }
        else if ( token == "-mode=dedicated" )
        {
            m_WorkMode = WorkerSettings::DEDICATED;
            m_OverrideWorkMode = true;
            continue;
        }
        else if ( token == "-mode=proportional" )
        {
            m_WorkMode = WorkerSettings::PROPORTIONAL;
            m_OverrideWorkMode = true;
            continue;
        }
        #if defined( __WINDOWS__ )
            else if ( token.BeginsWith( "-minfreememory=" ) )
            {
                uint32_t num( 0 );
                PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
                PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
                if ( sscanf( token.Get() + 15, "%u", &num ) == 1 )
                PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
                PRAGMA_DISABLE_POP_MSVC // 4996
                {
                    m_MinimumFreeMemoryMiB = num;
                }
                continue;
            }
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
            else if ( token == "-debug" )
            {
                Env::ShowMsgBox( "FBuildWorker", "Please attach debugger and press ok\n\n(-debug command line used)" );
                continue;
            }
        #endif

        ShowUsageError();
        return false;
    }

    return true;
}

// ShowUsageError
//------------------------------------------------------------------------------
void FBuildWorkerOptions::ShowUsageError()
{
    const char * msg = "FBuildWorker.exe - " FBUILD_VERSION_STRING "\n"
                       "Copyright 2012-2021 Franta Fulin - https://www.fastbuild.org\n"
                       "\n"
                       "Command Line Options:\n"
                       "---------------------------------------------------------------------------\n"
                       " -console\n"
                       "        (Windows/OSX) Operate from console instead of GUI.\n"
                       " -cpus=<n|-n|n%>   Set number of CPUs to use:\n"
                       "        -  n : Explicit number.\n"
                       "        - -n : Num CPU Cores-n.\n"
                       "        - n% : % of CPU Cores.\n"
                       " -debug\n"
                       "        (Windows) Break at startup, to attach debugger.\n"
                       " -mode=<disabled|idle|dedicated|proportional>\n"
                       "        Set work mode:\n"
                       "        - disabled : Don't accept any work.\n"
                       "        - idle : Accept work when PC is idle.\n"
                       "        - dedicated : Accept work always.\n"
                       "        - proportional : Accept work proportional to free CPUs.\n"
                       " -minfreememory <MiB>\n"
                       "        Set minimum free memory (MiB) required to accept work.\n"
                       " -nosubprocess\n"
                       "        (Windows) Don't spawn a sub-process worker copy.\n"
                       "---------------------------------------------------------------------------\n"
                       ;

    #if defined( __WINDOWS__ )
        ::MessageBox( nullptr, msg, "FBuildWorker - Bad Command Line", MB_ICONERROR | MB_OK );
    #else
        printf( "%s", msg );
        (void)msg; // TODO:MAC Fix missing MessageBox
        (void)msg; // TODO:LINUX Fix missing MessageBox
    #endif
}

//------------------------------------------------------------------------------
