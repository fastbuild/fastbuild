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

struct StringRoamer
{
    size_t index = 0;
    size_t length = 0;
    const char* position = nullptr;

    bool ReadChar()
    {
        if (index >= length)
        {
            return false;
        }
        ++position;
        ++index;
        return true;
    }
};

// FindLineInSourceFile - Takes as parameter the preprocessed data and the error line number in the preprocessed file. Return the error line number in the source file.
//------------------------------------------------------------------------------
static void FindLineInSourceFile( Array<uint32_t> & lineNumbersPreprocessedFile, StringRoamer preprocessedDataRoamer, Array<uint32_t> & lineNumbersSourceFile )
{
    uint32_t currentLine = 1;
    uint32_t lastIncludeLine = 1;
    uint32_t numberOfLastInclude = 0;

    for (uint32_t lineNumber : lineNumbersPreprocessedFile)
    {
        while (currentLine < lineNumber)
        {
            char c = *(preprocessedDataRoamer.position);
            if (c == '\n')
            {
                currentLine++;
            }
            else if (c == '#')
            {
                if (preprocessedDataRoamer.ReadChar()) 
                {
                    c = *(preprocessedDataRoamer.position);
                }
                else
                {
                    FLOG_ERROR("Could not find the error line number in preprocessed file");
                    return;
                }
                // Avoid #pragma and # in instructions
                if (c == ' ')
                {
                    lastIncludeLine = currentLine;
                    // Skip the empty space
                    if (preprocessedDataRoamer.ReadChar())
                    {
                        c = *(preprocessedDataRoamer.position);
                    }
                    else
                    {
                        FLOG_ERROR("Could not find the error line number in preprocessed file");
                        return;
                    }
                    // Find the number after the #
                    numberOfLastInclude = 0;
                    while (c != ' ')
                    {
                        // Do the conversion from char to uint
                        numberOfLastInclude = numberOfLastInclude * 10 + (c - '0');
                        if (preprocessedDataRoamer.ReadChar())
                        {
                            c = *(preprocessedDataRoamer.position);
                        }
                        else
                        {
                            FLOG_ERROR("Could not find the error line number in preprocessed file");
                            return;
                        }
                    }
                }
            }
            else
            {
                // Normal character part of token
            }
            if (!preprocessedDataRoamer.ReadChar())
            {
                FLOG_ERROR("Could not find the error line number in preprocessed file");
                return;
            }
        }
        lineNumbersSourceFile.Append(lineNumber - (lastIncludeLine + 1) + numberOfLastInclude);
    }

    return;
}

// HandleWarningsClangTidy
//------------------------------------------------------------------------------
void FileNode::HandleWarningsClangTidy( Job* job, const AString& name, const AString& data )
{
    AString newWarningMessage;
    // Avoid the client who wants to print remote work to process again the error line number
    // This part of the function workaround this issue : https://github.com/llvm/llvm-project/issues/62405
    if ( ( job->GetDistributionState() == Job::DIST_NONE || job->GetDistributionState() == Job::DIST_BUILDING_LOCALLY ) )
    {
        // Get preprocessed file data in a buffer
        const char* preprocessedData = static_cast<const char*>( job->GetData() );
        size_t preprocessedDataSize = job->GetDataSize() - job->GetExtraDataSize();

        // Handle compressed data
        Compressor c; // scoped here so we can access decompression buffer
        if ( job->IsDataCompressed() )
        {
            VERIFY( c.Decompress( preprocessedData ) );
            preprocessedData = static_cast<const char*>( c.GetResult() );
            preprocessedDataSize = c.GetResultSize() - job->GetExtraDataSize();
        }

        // Parse the warning message to get the error line numbers in preprocessed file
        // The warning message looks like this : "path\to\the\file:errorLineNumber:errorColumnNumber: warning: description of what is wrong"
        // There may be several concatenated warnings messages
        // It works both on Windows and Unix systems
        Array< AString > tokens;
        data.Tokenize( tokens, ':' );
        Array< uint32_t > indexes;
        bool foundWarning = false;
        for (uint32_t tokenIndex = 2; tokenIndex < tokens.GetSize(); tokenIndex++)
        {
            // The error line number is just before the "warning" string which is not at the beginning
            if (tokens[tokenIndex].Find("warning"))
            {
                foundWarning = true;
                indexes.Append(tokenIndex - 2);
            }
        }

        if ( foundWarning )
        {
            // Get the error line numbers and convert them to an integer
            Array<uint32_t> errorLineNumbersInteger;
            for (uint32_t index : indexes) {
                AString& errorLineNumberString = tokens[index];
                uint32_t errorLineNumberInteger = 0;
                if (errorLineNumberString.Scan("%d", &errorLineNumberInteger) != 1)
                {
                    FLOG_ERROR("Error: could not convert the error line number from AString to int");
                    return;
                }
                errorLineNumbersInteger.Append(errorLineNumberInteger);
            }

            // Read buffer to calculate the error line numbers from source file
            Array<uint32_t> lineNumbersFromSourceFile;
            StringRoamer preprocessedDataRoamer = { 0, preprocessedDataSize, preprocessedData };
            FindLineInSourceFile(errorLineNumbersInteger, preprocessedDataRoamer, lineNumbersFromSourceFile);

            // Modify error line numbers
            uint32_t currentIndex = 0;
            for (uint32_t index : indexes) {
                tokens[index].Format("%d", lineNumbersFromSourceFile[currentIndex]);
                currentIndex++;
            }

            // Reform the warning message
            for (uint32_t tokenIndex = 0; tokenIndex < tokens.GetSize(); tokenIndex++)
            {
                newWarningMessage += tokens[tokenIndex];
                // Does not add ":" after the last element of the message
                if (tokenIndex < tokens.GetSize() - 1)
                {
                    newWarningMessage += ":";
                }
            }
        }
    }

    constexpr const char* clangTidyWarningString = " warning:";
    if ( newWarningMessage.IsEmpty() )
    {
        return HandleWarnings( job, name, data, clangTidyWarningString );
    }
    else 
    {
        return HandleWarnings( job, name, newWarningMessage, clangTidyWarningString );
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
