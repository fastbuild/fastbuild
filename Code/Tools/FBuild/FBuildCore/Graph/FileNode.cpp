// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

#include <string.h> // for strstr

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileNode::FileNode()
    : Node( Node::FILE_NODE )
{
    m_LastBuildTimeMs = 1; // very little work required
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool FileNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ )
{
    ASSERT( false ); // Should never get here
    return false;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FileNode::~FileNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult FileNode::DoBuild( Job * /*job*/ )
{
    ASSERT( m_Name.EndsWith( "\\" ) == false );
    #if defined( __WINDOWS__ )
        ASSERT( ( m_Name.FindLast( ':' ) == nullptr ) ||
                ( m_Name.FindLast( ':' ) == ( m_Name.Get() + 1 ) ) );
    #endif

    // NOTE: Not calling RecordStampFromBuiltFile as this is not a built file
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    // Don't assert m_Stamp != 0 as input file might not exist
    return NODE_RESULT_OK;
}

// HandleWarningsMSVC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsMSVC( Job * job, const AString & name, const AString & data )
{
    constexpr const char * msvcWarningString = ": warning ";  // string is ok even in non-English
    return HandleWarnings( job, name, data, msvcWarningString );
}

// HandleWarningsClangCl
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangCl( Job * job, const AString & name, const AString & data )
{
    constexpr const char * clangWarningString = " warning:";
    return HandleWarnings( job, name, data, clangWarningString );
}

struct SourceLocation final
{
    AString filename;
    uint32_t lineNum;
};

// FixSourceLocations - Modify input line numbers taking #line directives into account
//------------------------------------------------------------------------------
static void FixSourceLocations( Array< SourceLocation > & sourceLocs, const char* source, size_t sourceLen )
{
    for ( SourceLocation& sourceLoc : sourceLocs )
    {
        uint32_t currentLine = 1;
        uint32_t lastIncludeLine = 1;
        uint32_t numberOfLastInclude = 0;

        const char* cursor = source;
        const char* end = source + sourceLen;

        auto ReadChar = [&] {
            if ( cursor >= end )
            {
                return false;
            }

            ++cursor;
            return true;
        };

        AStackString filename;

        while ( currentLine < sourceLoc.lineNum )
        {
            char c = *cursor;
            if ( c == '\n' )
            {
                ++currentLine;
            }
            else if ( c == '#' )
            {
                if ( ReadChar() )
                {
                    c = *cursor;
                }
                else
                {
                    FLOG_ERROR( "Could not find the error line number in preprocessed file" );
                    return;
                }

                // Avoid #pragma and # in instructions
                if ( c == ' ' )
                {
                    lastIncludeLine = currentLine;
                    // Skip the empty space
                    if ( ReadChar() )
                    {
                        c = *cursor;
                    }
                    else
                    {
                        FLOG_ERROR( "Could not find the error line number in preprocessed file" );
                        return;
                    }

                    // Find the number after the #
                    numberOfLastInclude = 0;
                    while ( c != ' ' )
                    {
                        numberOfLastInclude = numberOfLastInclude * 10 + (c - '0');
                        if ( ReadChar() )
                        {
                            c = *cursor;
                        }
                        else
                        {
                            FLOG_ERROR( "Could not find the error line number in preprocessed file" );
                            return;
                        }
                    }

                    ++cursor; // skip ' '
                    ASSERT(*cursor == '"');
                    ++cursor; // skip '"'

                    const char* filenameBegin = cursor;
                    while (*cursor != '"')
                    {
                        ++cursor;
                    }
                    const char* filenameEnd = cursor;

                    filename.Assign(filenameBegin, filenameEnd);
                }
            }

            if ( !ReadChar() )
            {
                FLOG_ERROR( "Could not find the error line number in preprocessed file" );
                return;
            }
        }

        sourceLoc.lineNum = sourceLoc.lineNum - (lastIncludeLine + 1) + numberOfLastInclude;
        PathUtils::FixupFilePath( filename );
        sourceLoc.filename = filename;
    }

    return;
}

// HandleDiagnosticsClangTidy
//------------------------------------------------------------------------------
void FileNode::HandleDiagnosticsClangTidy( Job* job, const AString& name, const AString& data )
{
    if ( data.IsEmpty() )
    {
        return;
    }

    AString newWarningMessage;
    // Avoid the client who wants to print remote work to process again the error line number
    // This part of the function workaround this issue : https://github.com/llvm/llvm-project/issues/62405
    if ( ( job->GetDistributionState() == Job::DIST_NONE || job->GetDistributionState() == Job::DIST_BUILDING_LOCALLY ) )
    {

        Array< AString > lines;
        data.Tokenize( lines, '\n' );

        Array< SourceLocation > sourceLocs;
        Array< uint32_t > fixupLineIndices;
        for (uint32_t i = 0; i < lines.GetSize(); ++i)
        {
            // Parse the warning message to get the error line numbers in preprocessed file
            // The warning message looks like this : "path\to\the\file:errorLineNumber:errorColumnNumber: warning: description of what is wrong"
            // There may be several concatenated diagnostic messages
            // It works both on Windows and Unix systems

            AString& line = lines[i];

            // The error line number is just before the "warning" string which is not at the beginning
            const char* diagnosticBegin = nullptr;
            diagnosticBegin = line.Find(": error:");
            diagnosticBegin = diagnosticBegin ? diagnosticBegin : line.Find(": warning:");
            diagnosticBegin = diagnosticBegin ? diagnosticBegin : line.Find(": note:");

            if (diagnosticBegin != nullptr)
            {
                fixupLineIndices.Append(i);

                // Skip column number
                const char* cursor = diagnosticBegin - 1; // skip starting ':'
                while (*cursor >= '0' && *cursor <= '9')
                {
                    --cursor;
                }
                ASSERT(*cursor == ':');
                diagnosticBegin = cursor;
                --cursor; // skip ':'

                // Read the line number backwards
                uint32_t lineNumber = 0;
                uint32_t power = 1;
                while (*cursor >= '0' && *cursor <= '9')
                {
                    uint32_t n = ( *cursor - uint32_t('0') );
                    lineNumber += power * n;
                    power *= 10;
                    --cursor;
                }
                ASSERT(*cursor == ':');

                sourceLocs.Append(SourceLocation{ AString(), lineNumber } );
                // Remove original filename and line number from line, we will add it back later
                line.Assign( diagnosticBegin, line.GetEnd() );
            }
        }

        if ( !fixupLineIndices.IsEmpty() )
        {
            const char* preprocessedData = static_cast<const char*>( job->GetData() );
            size_t preprocessedDataSize = job->GetDataSize() - job->GetExtraDataSize();

            Compressor c;
            if ( job->IsDataCompressed() )
            {
                VERIFY( c.Decompress( preprocessedData ) );
                preprocessedData = static_cast<const char*>( c.GetResult() );
                preprocessedDataSize = c.GetResultSize() - job->GetExtraDataSize();
            }

            FixSourceLocations( sourceLocs, preprocessedData, preprocessedDataSize );

            for (uint32_t i = 0, j = 0; i < lines.GetSize(); ++i)
            {
                if ( j < fixupLineIndices.GetSize() && i == fixupLineIndices[j] )
                {
                    AString& filename = sourceLocs[j].filename;
                    if ( !PathUtils::IsFullPath( filename ) )
                    {
                        filename = PathUtils::GetFullPath(filename);
                    }
                    newWarningMessage += filename;
                    newWarningMessage += ":";
                    newWarningMessage.AppendFormat( "%d", sourceLocs[j].lineNum );

                    ++j;
                }
                newWarningMessage += lines[i];
                newWarningMessage += "\n";
            }
        }
    }

    if ( newWarningMessage.IsEmpty() )
    {
        constexpr const char* clangTidyWarningString = " warning:";
        return HandleWarnings( job, name, data, clangTidyWarningString );
    }
    else 
    {
        constexpr const char* clangTidyErrorString = " error:";
        const bool treatAsWarnings = newWarningMessage.Find(clangTidyErrorString) == nullptr;
        DumpOutput( job, name, newWarningMessage, treatAsWarnings );
    }
}

// HandleWarnings
//------------------------------------------------------------------------------
void FileNode::HandleWarnings( Job * job, const AString & name, const AString & data, const char * warningString )
{
    if ( data.IsEmpty() )
    {
        return;
    }

    // Are there any warnings?
    if ( data.Find( warningString ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, name, data, treatAsWarnings );
    }
}

// HandleWarningsClangGCC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangGCC( Job * job, const AString & name, const AString & data )
{
    if ( data.IsEmpty() )
    {
        return;
    }

    // Are there any warnings? (string is ok even in non-English)
    if ( data.Find( "warning: " ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, name, data, treatAsWarnings );
    }
}

// DumpOutput
//------------------------------------------------------------------------------
void FileNode::DumpOutput( Job * job, const AString & name, const AString & data, bool treatAsWarnings )
{
    if ( data.IsEmpty() == false )
    {
        Array< AString > exclusions( 2, false );
        exclusions.EmplaceBack( "Note: including file:" );
        exclusions.EmplaceBack( "#line" );

        AStackString<> msg;
        msg.Format( "%s: %s\n", treatAsWarnings ? "WARNING" : "PROBLEM", name.Get() );

        AString finalBuffer( data.GetLength() + msg.GetLength() );
        finalBuffer += msg;
        finalBuffer += data;

        Node::DumpOutput( job, finalBuffer, &exclusions );
    }
}

//------------------------------------------------------------------------------
