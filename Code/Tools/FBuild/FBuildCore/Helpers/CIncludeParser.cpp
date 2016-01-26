// CIncludeParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CIncludeParser.h"

#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <string.h>
#include "Core/Process/Process.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"
#include "Core/Env/Env.h"

//------------------------------------------------------------------------------
CIncludeParser::CIncludeParser()
    : m_LastCRC1( 0 )
    , m_CRCs1( 4096, true )
    , m_LastCRC2( 0 )
    , m_CRCs2( 4096, true )
    , m_Includes( 4096, true )
#ifdef DEBUG
    , m_NonUniqueCount( 0 )
#endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CIncludeParser::~CIncludeParser() = default;

AStackString< 256 >  CIncludeParser::ms_ShowIncludesMarker( "Note: including file:" ); // Default value. The default value is mainly for unittests.
bool                 CIncludeParser::ms_ShowIncludesMarkerDetected;
Mutex                CIncludeParser::ms_ShowIncludesMarkerMutex;

bool CIncludeParser::DetectMS_ShowIncludesMarker(const AString & compiler)
{
    // Avoid locking once the pattern is detected
    if (ms_ShowIncludesMarkerDetected)
        return true;

    MutexHolder lock(ms_ShowIncludesMarkerMutex);
    if (ms_ShowIncludesMarkerDetected)
        return true;


    FileStream tmpFile;
    AStackString< 256 > tmpFilenameHeader;
    AStackString< 64 > dummyHeader( "#pragma once" );
    WorkerThread::CreateTempFilePath( "showincludedetect.h", tmpFilenameHeader );
    WorkerThread::CreateTempFile( tmpFilenameHeader, tmpFile );
    if (! tmpFile.Write( dummyHeader.Get(), dummyHeader.GetLength() ) )
    {
        OUTPUT( "Failed to write to temp file '%s' (error %u)", tmpFilenameHeader.Get(), Env::GetLastErr() );
        return false;
    }
    tmpFile.Close();


    AStackString< 256 > tmpFilenameCPP;
    AStackString< 256 > tmpFilenameObj;
    AStackString< 64 > dummyCPP;
    dummyCPP.Format( "#include \"%s\"", tmpFilenameHeader.Get() );
    const char* includeDetectFilename = "showincludedetect.cpp";
    const size_t includeDetectLength = 21;
    WorkerThread::CreateTempFilePath( includeDetectFilename, tmpFilenameCPP );
    WorkerThread::CreateTempFilePath( "showincludedetect.obj", tmpFilenameObj );
    WorkerThread::CreateTempFile( tmpFilenameCPP, tmpFile );
    if (! tmpFile.Write( dummyCPP.Get(), dummyCPP.GetLength() ) )
    {
        OUTPUT( "Failed to write to temp file '%s' (error %u)", tmpFilenameCPP.Get(), Env::GetLastErr() );
        return false;
    }
    tmpFile.Close();

    AStackString< 256 > args;
    args.Format( "/c %s /Fo%s /showIncludes /nologo", tmpFilenameCPP.Get(), tmpFilenameObj.Get() );

    const char * environmentString = ( FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr );
    Process compilerProcess;
    if ( false == compilerProcess.Spawn( compiler.Get(),
        args.Get(),
        nullptr,
        environmentString ) )
    {
        OUTPUT( "Failed to spawn process (error 0x%x) to build '%s'\n", Env::GetLastErr(), tmpFilenameCPP.Get() );
        return false;
    }
    AutoPtr< char > out;
    uint32_t		outSize;
    AutoPtr< char > err;
    uint32_t		errSize;

    // capture all of the stdout and stderr
    compilerProcess.ReadAllData( out, &outSize, err, &errSize );

    // Get result
    int result = compilerProcess.WaitForExit();
    if ( result != 0 )
    {
        OUTPUT( "Failed to build %s Object (error 0x%x)\n", tmpFilenameCPP.Get(), result );

        return false;
    }

    FileIO::FileDelete(tmpFilenameCPP.Get());
    FileIO::FileDelete(tmpFilenameHeader.Get());
    FileIO::FileDelete(tmpFilenameObj.Get());

    ASSERT(out.Get()[outSize] == 0);
    const char* pos = out.Get();

    bool foundMarker = false;
    if (pos && strncmp(pos, includeDetectFilename, includeDetectLength) == 0)
    {
        // Change position to beginning of next line
        pos = strchr(pos + includeDetectLength, '\r');
        if(pos && pos[1] == '\n')
            pos += 2;
    }
    if(pos)
    {
        // Marker has the following form : 
        // <string> : <other string> :
        // Examples:
        //
        // French:
        // Remarque : inclusion du fichier : X:\A\B\C
        // English:
        // Note: including file: 
        // Look for two : symbols. Marker is in between the start of the line and the second : marker
        const char* firstMarkerSeparator = strchr(pos, ':');
        if (firstMarkerSeparator != nullptr)
        {
            const char* secondMarkerSeparator = strchr(firstMarkerSeparator + 1, ':');
            if (secondMarkerSeparator != nullptr)
            {
                // Now that we've found the marker, we will just validate that we've really found it by validating the path of the include.
                // This is a little bit parano but help insuring that the parsing was really correct.
                const char* afterMarker = secondMarkerSeparator + 1;

                // Skip all spaces.
                while(*afterMarker == ' ')
                    ++afterMarker;

                // Find end of line
                const char* endLine = strchr(afterMarker, '\n');
                if (endLine != nullptr)
                {
                    endLine = endLine[-1] == '\r' ? endLine - 1 : endLine;

                    AStackString< 256> tmpPath(afterMarker, endLine);
                    if (tmpPath.CompareI(tmpFilenameHeader) == 0)
                    {
                        // Found the same include! Everything is fine
                        ms_ShowIncludesMarker.Assign(pos, secondMarkerSeparator + 1);
                        foundMarker = true;
                    }
                }
            }
        }
    }

    if ( !foundMarker )
    {
        OUTPUT( "Error while parsing result for /showincludes marker: '%s'", out.Get() );
        return false;
    }

    ms_ShowIncludesMarkerDetected = true;
    return true;
}

// Parse
//------------------------------------------------------------------------------
bool CIncludeParser::ParseMSCL_Output( const char * compilerOutput,
                                       size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    AStackString< 256 > rawIncludePath;
    AStackString< 256 > tmpIncludePath;
    const char * pos = compilerOutput;
    const char * endLine = nullptr;
    for (;;)
    {
        // Look for next include information marker.
        const char* nextIncludeMarker = strstr(pos, ms_ShowIncludesMarker.Get());
        if ( !nextIncludeMarker )
        {
            break;  // No more includes
        }

        // find end of the line
        endLine = strchr( nextIncludeMarker, '\n' );
        if ( !endLine )
        {
            break; // end of output
        }
        endLine = endLine[-1] == '\r' ? endLine - 1 : endLine;

        // Find start of include - skipping indentation
        const char* nextInclude = nextIncludeMarker + ms_ShowIncludesMarker.GetLength();
        while( *nextInclude == ' ' )
            ++nextInclude;

        // Assign raw includePath before any transformation
        rawIncludePath.Assign( nextInclude, endLine );
        if ( PathUtils::IsFullPath( rawIncludePath ) )
        {
            AddInclude( rawIncludePath.Get(), rawIncludePath.Get() + rawIncludePath.GetLength() );
        }
        else
        {
            // We've got a relative path...
            NodeGraph::CleanPath( rawIncludePath, tmpIncludePath );
            bool fileExists = FileIO::FileExists( tmpIncludePath.Get() );
            ASSERT( fileExists );
            if ( fileExists ) // Something is wrong with the parsing
            {
                AddInclude( tmpIncludePath.Get(), tmpIncludePath.Get() + tmpIncludePath.GetLength() );
            }
            else
            {
                FLOG_WARN( "Found non existing file when parsing dependencies: %s. Skipping it", tmpIncludePath.Get() );
            }
        }

        // Advance to next line
        pos = endLine + 1;
    }

    return true;
}

// Parse
//------------------------------------------------------------------------------
bool CIncludeParser::ParseMSCL_Preprocessed( const char * compilerOutput,
                                             size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    const char * pos = compilerOutput;

    for (;;)
    {
        pos = strstr( pos, "#line 1 " );
        if ( !pos )
        {
            break;
        }

        const char * lineStart = pos;
        pos += 8;

        // search backwards for start of line
    searchForLineStart:
        // special case for first line (prevent buffer underread)
        if ( lineStart == compilerOutput )
        {
            goto foundInclude;
        }

        // skip whitespace
        --lineStart;
        if ( ( *lineStart == ' ' ) || ( *lineStart == '\t' ) )
        {
            goto searchForLineStart;
        }

        // wrapped to previous line?
        if ( *lineStart == '\n' )
        {
            goto foundInclude;
        }

        // hit some non-whitespace before the #line
        continue; // look for another #line

    foundInclude:

        // go to opening quote
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false;
        }
        pos++;

        const char * incStart = pos;

        // find end of line
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false;
        }

        const char * incEnd = pos;

        AddInclude( incStart, incEnd );
    }

    return true;
}

// ParseToNextLineStaringWithHash
//------------------------------------------------------------------------------
/*static*/ void CIncludeParser::ParseToNextLineStartingWithHash( const char * & pos )
{
    for (;;)
    {
        pos = strchr( pos, '#' );
        if ( pos )
        {
            // Safe to index -1 because # as first char is handled as a
            // special case to avoid having it in this critical loop
            const char prevC = pos[ -1 ];
            if ( ( prevC  == '\n' ) || ( prevC  == '\r' ) )
            {
                return;
            }
            ++pos;
            continue;
        }
        return;
    }
}

// Parse
//------------------------------------------------------------------------------
bool CIncludeParser::ParseGCC_Preprocessed( const char * compilerOutput,
                                            size_t compilerOutputSize )
{
    // we require null terminated input
    ASSERT( compilerOutput[ compilerOutputSize ] == 0 );
    (void)compilerOutputSize;

    const char * pos = compilerOutput;

    // special case for include on first line
    // (out of loop to keep loop logic simple)
    if ( pos[ 0 ] == '#' )
    {
        ++pos;
        goto possibleInclude;
    }

    for (;;)
    {
        ParseToNextLineStartingWithHash( pos );
        if ( !pos )
        {
            break;
        }
        ++pos;
    possibleInclude:
        if ( *pos == ' ' )
        {
            ++pos;
            goto foundInclude;
        }
        if ( strncmp( pos, "line ", 5 ) == 0 )
        {
            pos += 5;
            goto foundInclude;
        }
        continue; // some other directive we don't care about

    foundInclude:

        // skip number
        for ( ;; )
        {
            char c = * pos;
            if ( ( c >= '0' ) && ( c <= '9' ) )
            {
                pos++;
                continue;
            }
            break; // non numeric
        }

        // single space
        if ( *pos != ' ' )
        {
            continue;
        }
        pos++;

        // opening quote
        if ( *pos != '"' )
        {
            continue;
        }
        pos++;

        // ignore special case GCC "<built-in>" and "<command line>"
        if ( *pos == '<' )
        {
            continue;
        }

        const char * lineStart = pos;

        // find end of line
        pos = strchr( pos, '"' );
        if ( !pos )
        {
            return false; // corrupt input
        }

        const char * lineEnd = pos;

        // ignore GCC paths
        const char lastChar( lineEnd[ -1 ] );
        if ( ( lastChar == NATIVE_SLASH ) || ( lastChar == OTHER_SLASH ) )
        {
            continue;
        }

        AddInclude( lineStart, lineEnd );
    }

    return true;
}

// SwapIncludes
//------------------------------------------------------------------------------
void CIncludeParser::SwapIncludes( Array< AString > & includes )
{
    m_Includes.Swap( includes );
}

// AddInclude
//------------------------------------------------------------------------------
void CIncludeParser::AddInclude( const char * begin, const char * end )
{
    #ifdef DEBUG
        m_NonUniqueCount++;
    #endif

    // quick check
    uint32_t crc1 = xxHash::Calc32( begin, (size_t)( end - begin ) );
    if ( crc1 == m_LastCRC1 )
    {
        return;
    }
    m_LastCRC1 = crc1;
    if ( m_CRCs1.Find( crc1 ) )
    {
        return;
    }
    m_CRCs1.Append( crc1 );

    // robust check
    AStackString< 256 > include( begin, end );
    AStackString< 256 > cleanInclude;
    NodeGraph::CleanPath( include, cleanInclude );
    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Windows and OSX are case-insensitive
        AStackString<> lowerCopy( cleanInclude );
        lowerCopy.ToLower();
        uint32_t crc2 = xxHash::Calc32( lowerCopy );
    #else
        // Linux is case-sensitive
        uint32_t crc2 = xxHash::Calc32( cleanInclude );
    #endif
    if ( crc2 == m_LastCRC2 )
    {
        return;
    }
    m_LastCRC2 = crc2;
    if ( m_CRCs2.Find( crc2 ) == nullptr )
    {
        m_CRCs2.Append( crc2 );
        m_Includes.Append( cleanInclude );
    }
}

//------------------------------------------------------------------------------
