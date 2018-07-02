// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildWorkerOptions.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/Env/Env.h"

// system
#include <stdio.h>
#if defined( __WINDOWS__ )
    #include <windows.h>
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
    m_WorkMode( WorkerSettingsNode::WHEN_IDLE ),
    m_OverrideStartMinimized( false ),
    m_StartMinimized( false ),
    m_OverrideSandboxEnabled( false),
    m_SandboxEnabled( false ),
    m_OverrideSandboxExe( false),
    m_OverrideSandboxArgs( false),
    m_OverrideSandboxTmp( false ),
    m_OverrideWorkerTags( false ),
    m_ConsoleMode( false )
{
    #ifndef __WINDOWS__
        m_ConsoleMode = true; // TODO:OSX Support GUI mode
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
    for ( const AString * it=tokens.Begin(); it != end; ++it )
    {
        const AString & token = *it;
        if ( token == "-console" )
        {
            m_ConsoleMode = true;
            #if defined( __WINDOWS__ )
                m_UseSubprocess = false;
            #endif
            continue;
        }
        else if ( token.BeginsWith( "-cpus=" ) )
        {
            int32_t numCPUs = Env::GetNumProcessors();
            int32_t num( 0 );
            if ( sscanf( token.Get() + 6, "%i", &num ) == 1 )
            {
                if ( token.EndsWith( '%' ) )
                {
                    num = (int32_t)( numCPUs * (float)num / 100.0f );
                    m_CPUAllocation = Math::Clamp( num, 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                else if ( num > 0 )
                {
                    m_CPUAllocation = Math::Clamp( num, 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                else if ( num < 0 )
                {
                    m_CPUAllocation = Math::Clamp( ( numCPUs + num ), 1, numCPUs );
                    m_OverrideCPUAllocation = true;
                    continue;
                }
                // problem... fall through
            }
            // problem... fall through
        }
        else if ( token == "-mode=disabled" )
        {
            m_WorkMode = WorkerSettingsNode::DISABLED;
            m_OverrideWorkMode = true;
            continue;
        }
        else if ( token == "-mode=idle" )
        {
            m_WorkMode = WorkerSettingsNode::WHEN_IDLE;
            m_OverrideWorkMode = true;
            continue;
        }
        else if ( token == "-mode=dedicated" )
        {
            m_WorkMode = WorkerSettingsNode::DEDICATED;
            m_OverrideWorkMode = true;
            continue;
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
        else if ( token == "-min" )
        {
            m_StartMinimized = true;
            m_OverrideStartMinimized = true;
            continue;
        }
        else if ( token == "-nomin" )
        {
            m_StartMinimized = false;
            m_OverrideStartMinimized = true;
            continue;
        }
        else if ( token == "-sandbox" )
        {
            m_SandboxEnabled = true;
            m_OverrideSandboxEnabled = true;
            continue;
        }
        else if ( token == "-nosandbox" )
        {
            m_SandboxEnabled = false;
            m_OverrideSandboxEnabled = true;
            continue;
        }
        else if ( token.BeginsWith( "-sandboxexe=" ) )
        {
            AStackString<> sandboxExe( token.Get() + 12 );
            Args::StripQuotes(
                sandboxExe.Get(),
                sandboxExe.Get() + sandboxExe.GetLength(),
                m_SandboxExe );
            m_OverrideSandboxExe = true;
            continue;
        }
        else if ( token.BeginsWith( "-sandboxargs=" ) )
        {
            AStackString<> sandboxArgs( token.Get() + 13 );
            Args::StripQuotes(
                sandboxArgs.Get(),
                sandboxArgs.Get() + sandboxArgs.GetLength(),
                m_SandboxArgs );
            m_OverrideSandboxArgs = true;
            continue;
        }
        else if ( token.BeginsWith( "-sandboxtmp=" ) )
        {
            AStackString<> sandboxTmp( token.Get() + 12 );
            Args::StripQuotes(
                sandboxTmp.Get(),
                sandboxTmp.Get() + sandboxTmp.GetLength(),
                m_SandboxTmp );
            m_OverrideSandboxTmp = true;
            continue;
        }
        else if ( token.BeginsWith( "-T" ) )
        {
            AStackString<> tagStr( token.Get() + 2 );
            m_WorkerTags.ParseAndAddTag( tagStr );
            m_OverrideWorkerTags = true;
            continue;
        }

        ShowUsageError();
        return false;
    }

    // always set valid, even if empty container
    m_WorkerTags.SetValid( true );

    return true;
}

// ShowUsageError
//------------------------------------------------------------------------------
void FBuildWorkerOptions::ShowUsageError()
{
    const char * msg = "FBuildWorker.exe - " FBUILD_VERSION_STRING "\n"
                       "Copyright 2012-2018 Franta Fulin - http://www.fastbuild.org\n"
                       "\n"
                       "Command Line Options:\n"
                       "------------------------------------------------------------\n"
                       "-cpus=[n|-n|n%] : Set number of CPUs to use.\n"
                       "                n : Explicit number.\n"
                       "                -n : NUMBER_OF_PROCESSORS-n.\n"
                       "                n% : % of NUMBER_OF_PROCESSORS.\n"
                       "\n"
                       "-mode=[disabled|idle|dedicated] : Set work mode.\n"
                       "                disabled : Don't accept any work.\n"
                       "                idle : Accept work when PC is idle.\n"
                       "                dedicated : Accept work always.\n"
                       "\n"
                       "-min : Start minimized.\n"
                       "-nomin : Don't start minimized.\n"
                       "\n"
                       "-sandbox : Enable work execution sandbox.\n"
                       "-nosandbox : Disable work execution sandbox.\n"
                       "\n"
                       "-sandboxexe=path : Set sandbox executable.\n"
                       "                Example relative path : winc.exe\n"
                       "                Example absolute path : \"C:\\fastbuild_data\\bin\\win64\\winc.exe\"\n"
                       "\n"
                       " -sandboxargs=args : Set sandbox args.\n"
                       "                Example args : \"--affinity 1 --memory 10485760\"\n"
                       "\n"
                       "-sandboxtmp=path : Set sandbox tmp dir.\n"
                       "                Example relative path : sandbox\n"
                       "                Example absolute path : \"C:\\ProgramData\\fbuild\\sandbox\"\n"
                       "\n"
                       "-Ttag : Set a worker tag.\n"
                       "                You may specify one or more -Ttag entries.\n"
                       "                Example : -TTopDownMemory -TOS=Win-7-64\n"
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
