// LightCache
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "LightCache.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Mutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// Include Type
//------------------------------------------------------------------------------
enum class IncludeType : uint8_t
{
    ANGLE,      // #include <file.h>
    QUOTE,      // #include "file.h"
    MACRO,      // #include MACRO_H
};

// IncludedFile
//------------------------------------------------------------------------------
class IncludedFile
{
public:
    class Include
    {
    public:
        Include( const AString & include, IncludeType type )
            : m_Include( include )
            , m_Type( type )
        {}

        AString                     m_Include;
        IncludeType                 m_Type;
    };

    uint64_t                        m_FileNameHash;
    AString                         m_FileName;
    bool                            m_Exists;
    uint64_t                        m_ContentHash;
    Array< Include >                m_Includes;

    inline bool operator == ( const AString & fileName ) const      { return ( m_FileName == fileName ); }
    inline bool operator == ( const IncludedFile & other ) const    { return ( ( m_FileNameHash == other.m_FileNameHash ) && ( m_FileName == other.m_FileName ) ); }
    inline bool operator <  ( const IncludedFile & other ) const    { return ( m_FileName < other.m_FileName ); }
};

// IncludeDefine
//------------------------------------------------------------------------------
class IncludeDefine
{
public:
    IncludeDefine( const AString & macro, const AString & include, IncludeType type )
        : m_Macro( macro )
        , m_Include( include )
        , m_Type( type )
    {}

    AString                         m_Macro;
    AString                         m_Include;
    IncludeType                     m_Type;
};

// IncludedFileBucket
//------------------------------------------------------------------------------
class IncludedFileBucket
{
public:
    IncludedFileBucket()
        : m_Mutex()
        , m_Files( 1024, true )
    {}
    void Destruct()
    {
        for ( IncludedFile * file : m_Files )
        {
            FDELETE file;
        }
        m_Files.Destruct();
    }

    Mutex                   m_Mutex;
    Array< IncludedFile * > m_Files;
};
#define LIGHTCACHE_NUM_BUCKETS 128
static IncludedFileBucket g_AllIncludedFiles[ LIGHTCACHE_NUM_BUCKETS ];

// CONSTRUCTOR
//------------------------------------------------------------------------------
LightCache::LightCache()
    : m_IncludePaths( 32, true )
    , m_AllIncludedFiles( 2048, true )
    , m_IncludeStack( 32, true )
    , m_ProblemParsing( false )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LightCache::~LightCache()
{
    for ( const IncludeDefine * def : m_IncludeDefines )
    {
        FDELETE def;
    }
}

// Hash
//------------------------------------------------------------------------------
bool LightCache::Hash( ObjectNode * node,
                       const AString & compilerArgs,
                       uint64_t & outSourceHash,
                       Array< AString > & outIncludes )
{
    PROFILE_FUNCTION

    ProjectGeneratorBase::ExtractIntellisenseOptions( compilerArgs,
                                                      "-I",
                                                      "/I",
                                                      m_IncludePaths,
                                                      false,    //escapeQuotes,
                                                      false );  //keepFullOption

    // Ensure all includes are slash terminated
    for ( AString & includePath : m_IncludePaths )
    {
        if ( includePath.EndsWith( NATIVE_SLASH ) || includePath.EndsWith( OTHER_SLASH ) )
        {
            continue;
        }
        includePath += NATIVE_SLASH;
    }

    const AString & rootFileName = node->GetSourceFile()->GetName();
    ProcessInclude( rootFileName, IncludeType::QUOTE );

    // Handle missing root file
    if ( m_AllIncludedFiles.IsEmpty() )
    {
        // TODO: Error
        outSourceHash = 0;
        return false;
    }

    // Was there a problem during parsing? (Some construct we don't know how to handle for example)
    if ( m_ProblemParsing )
    {
        outSourceHash = 0;
        return false;
    }

    // Create final hash and return includes
    const size_t numIncludes = m_AllIncludedFiles.GetSize();
    Array< uint64_t > hashes( numIncludes * 2, false );
    outIncludes.SetCapacity( numIncludes );
    for ( const IncludedFile * file : m_AllIncludedFiles )
    {
        hashes.Append( file->m_FileNameHash ); // Filename can change compilation result
        hashes.Append( file->m_ContentHash );
        outIncludes.Append( file->m_FileName );
    }
    outSourceHash = xxHash::Calc64( hashes.Begin(), hashes.GetSize() * sizeof( uint64_t ) );

    return true;
}

// ClearCachedFiles
//------------------------------------------------------------------------------
/*static*/ void LightCache::ClearCachedFiles()
{
    for ( IncludedFileBucket & bucket : g_AllIncludedFiles )
    {
        bucket.Destruct();
    }
}

// Parse
//------------------------------------------------------------------------------
void LightCache::Parse( IncludedFile * file, FileStream & f )
{
    ASSERT( f.IsOpen() );

    // Read all contents
    const size_t fileSize = f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( (uint32_t)fileSize );
    if ( f.Read( fileContents.Get(), fileSize ) != fileSize )
    {
        m_ProblemParsing = true;
        return;
    }
    f.Close();


    // Store hash of file
    file->m_ContentHash = xxHash::Calc64( fileContents );

    const char * pos = fileContents.Get();
    for (;;)
    {
        // skip leading whitespace
        SkipWhitespace( pos );
        if ( *pos == 0 )
        {
            break;
        }

        // did we hit the end of a line?
        if ( IsAtEndOfLine( pos ) )
        {
            // line is entirely whitepace
            SkipLineEnd( pos );
            continue;
        }

        const char c = *pos;

        // Is this a directive?
        if ( c == '#' )
        {
            if ( ParseDirective( *file, pos ) == false )
            {
                m_ProblemParsing = true;
                return;
            }
        }

        // block comment?
        if ( ( c == '/' ) && ( pos[ 1 ] == '*' ) )
        {
            SkipCommentBlock( pos );
        }

        // Advance to next line
        SkipToEndOfLine( pos );
        SkipLineEnd( pos );
    }
}

// ParseDirective
//------------------------------------------------------------------------------
bool LightCache::ParseDirective( IncludedFile & file, const char * & pos )
{
    // Skip '#' and whitespace
    ASSERT( *pos == '#' );
    pos++;
    SkipWhitespace( pos );

    // Handle directives we understand and care about
    if ( AString::StrNCmp( pos, "include", 7 ) == 0 )
    {
        return ParseDirective_Include( file, pos );
    }
    if ( AString::StrNCmp( pos, "define", 6 ) == 0 )
    {
        return ParseDirective_Define( file, pos );
    }
    else if ( AString::StrNCmp( pos, "import", 6 ) == 0 )
    {
        return ParseDirective_Import( file, pos );
    }

    // A directive we ignore
    return true;
}

// ParseDirective_Include
//------------------------------------------------------------------------------
bool LightCache::ParseDirective_Include( IncludedFile & file, const char * & pos )
{
    // skip "include" and whitespace
    ASSERT( AString::StrNCmp( pos, "include", 7 ) == 0 );
    pos += 7;
    SkipWhitespace( pos );

    // Get include string
    AStackString<> include;
    IncludeType includeType;
    if ( ( *pos == '"' ) || ( *pos == '<' ) )
    {
        // Looks like a normal include
        if ( ParseIncludeString( pos, include, includeType ) == false )
        {
            // We encountered an include we can't handle (using a macro for the path for example)
            return false;
        }

        file.m_Includes.Append( IncludedFile::Include{ include, includeType } );
        return true;
    }

    // Not a normal include - perhaps this is a macro?
    AStackString<> macroName;
    if ( ParseMacroName( pos, macroName ) == false )
    {
        return false; // We saw an unexpected sequence after the #include
    }

    // Store the macro include which will be resolved later
    file.m_Includes.Append( IncludedFile::Include{ macroName, IncludeType::MACRO } );
    return true;
}

// ParseDirective_Define
//------------------------------------------------------------------------------
bool LightCache::ParseDirective_Define( IncludedFile & /*file*/, const char * & pos )
{
    // skip "include" and whitespace
    ASSERT( AString::StrNCmp( pos, "define", 6 ) == 0 );
    pos += 6;
    SkipWhitespace( pos );

    // Get macro name
    AStackString<> macroName;
    if ( ParseMacroName( pos, macroName ) == false )
    {
        return false; // Unexpected macro form - we don't know how to handle this
    }

    SkipWhitespace( pos );

    // Is this defining an include path?
    AStackString<> include;
    IncludeType includeType;
    if ( ParseIncludeString( pos, include, includeType ) == false )
    {
        // Not an include - this isn't an error. We just don't care about this macro
        return true;
    }

    // Take not of the macro and the path it defines
    m_IncludeDefines.Append( FNEW( IncludeDefine( macroName, include, includeType ) ) );

    return true;
}

// ParseDirective_Import
//------------------------------------------------------------------------------
bool LightCache::ParseDirective_Import( IncludedFile & /*file*/, const char * & /*pos*/ )
{
    // We encountered an import directive, we can't handle them.
    return false;
}

// SkipCommentBlock
//------------------------------------------------------------------------------
void LightCache::SkipCommentBlock( const char * & pos )
{
    // Skip opening /*
    ASSERT( ( pos[ 0 ] == '/' ) && ( pos[ 1 ] == '*' ) );

    // Skip to closing*/
    for (;;)
    {
        const char thisChar = *pos;

        // end of data?
        if ( thisChar == 0 )
        {
            break;
        }

        // end of comment block?
        if ( ( thisChar == '*' ) && ( pos[ 1 ] == '/' ) )
        {
            pos +=2;
            break;
        }

        // part of comment
        ++pos;
    }
}

// ParseIncludeString
//------------------------------------------------------------------------------
bool LightCache::ParseIncludeString( const char * & pos,
                                     AString & outIncludePath,
                                     IncludeType & outIncludeType )
{
    // Does it start with a " or <
    if ( ( *pos != '"' ) && ( *pos != '<' ) )
    {
        // Not a valid include string
        return false;
    }

    outIncludeType = ( *pos == '<' ) ? IncludeType::ANGLE : IncludeType::QUOTE;

    const char * includeStart = ( pos + 1 );
    if ( SkipToEndOfQuotedString( pos ) == false )
    {
        return false;
    }
    const char * includeEnd = ( pos - 1 );
    outIncludePath.Assign( includeStart, includeEnd );
    return true;
}

// ParseMacroName
//------------------------------------------------------------------------------
bool LightCache::ParseMacroName( const char * & pos, AString & outMacroName )
{
    // Get macro name
    const char * macroNameStart = pos;
    char c = *macroNameStart;
    // Check for valid identifier start
    if ( !( ( c >= 'a' ) && ( c <= 'z' ) ) &&
         !( ( c >= 'A' ) && ( c <= 'Z' ) ) &&
         !( c == '_' ) )
    {
        return false; // Not a valid macro name so ignore it
    }
    ++pos;

    // Find end of macro name
    const char * macroNameEnd;
    for ( ;; )
    {
        c = *pos;
        if ( ( ( c >= 'a' ) && ( c <= 'z' ) ) ||
                ( ( c >= 'A' ) && ( c <= 'Z' ) ) ||
                ( ( c >= '0' ) && ( c <= '9' ) ) || 
                ( c == '_' ) )
        {
            ++pos;
            continue;
        }
        macroNameEnd = pos;
        break;
    }
    outMacroName.Assign( macroNameStart, macroNameEnd );
    return true;
}

// ProcessInclude
//------------------------------------------------------------------------------
void LightCache::ProcessInclude( const AString & include, IncludeType type )
{
    bool cyclic = false;
    const IncludedFile * file = nullptr;

    // Handle full paths
    if ( PathUtils::IsFullPath( include ) )
    {
        file = ProcessIncludeFromFullPath( include, cyclic );
    }
    else
    {
        // #include MACRO_H
        if ( type == IncludeType::MACRO )
        {

            // Find macro - expand each possible value
            // use index here, since iterators may be invalidated when recursing
            // get size each iteration, since size could have increased when recursing
            for ( size_t i=0; i<m_IncludeDefines.GetSize(); ++i )
            {
                const IncludeDefine * def = m_IncludeDefines[i];
                if ( def->m_Macro == include )
                {
                    ProcessInclude( def->m_Include, def->m_Type );
                }
            }
            return;
        }

        // From MSDN: http://msdn.microsoft.com/en-us/library/36k2cdd4.aspx

        if ( type == IncludeType::ANGLE )
        {
            // #include <file.h>

            // 1. Along the path that's specified by each /I compiler option.
            file = ProcessIncludeFromIncludePath( include, cyclic );

            // 2. When compiling occurs on the command line, along the paths that are specified by the INCLUDE environment variable.
            //if ( file == nullptr )
            //{
            //    ASSERT( false ); // TODO: Implement
            //}
        }
        else if ( type == IncludeType::QUOTE )
        {
            // #include "file.h"

            // 1. In the same directory as the file that contains the #include statement.
            // 2. In the directories of the currently opened include files, in the reverse order in which they were opened. The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
            file = ProcessIncludeFromIncludeStack( include, cyclic );

            // 3. Along the path that's specified by each /I compiler option.
            if ( file == nullptr )
            {
                file = ProcessIncludeFromIncludePath( include, cyclic );
            }

            // 4. Along the paths that are specified by the INCLUDE environment variable.
            //if ( file == nullptr )
            //{
            //    ASSERT( false ); // TODO: Implement
            //}
        }
    }

    if ( file == nullptr )
    {
        // Include not found. This is ok because:
        // a) The file might not be needed. If the include is within an inactive part of the file
        //    such as a comment or ifdef'd for example. If compilation succeeds, the file should
        //    not be part of our dependencies anyway, so this is ok.
        // b) The files is genuinely missing, in which case compilation will fail. If compilation
        //    fails then we won't bake the dependencies with the missing file, so this is ok.
        return;
    }

    // Avoid recursing into files we've already seen
    if ( cyclic )
    {
        return;
    }

    // Have we already seen this file before?
    if ( m_AllIncludedFiles.FindDeref( *file ) )
    {
        return;
    }

    // Take note of this included file
    m_AllIncludedFiles.Append( file );

    // Recurse
    m_IncludeStack.Append( file );
    for ( const IncludedFile::Include & inc : file->m_Includes )
    {
        ProcessInclude( inc.m_Include, inc.m_Type );
    }
    m_IncludeStack.Pop();
}

// ProcessIncludeFromFullPath
//------------------------------------------------------------------------------
const IncludedFile * LightCache::ProcessIncludeFromFullPath( const AString & include, bool & outCyclic )
{
    outCyclic = false;

    // Handle cyclic includes
    const IncludedFile ** found = m_IncludeStack.FindDeref( include );
    if ( found )
    {
        outCyclic = true;
        return *found;
    }

    const IncludedFile * file = FileExists( include );
    ASSERT( file );
    return file;
}

// ProcessIncludeFromIncludeStack
//------------------------------------------------------------------------------
const IncludedFile * LightCache::ProcessIncludeFromIncludeStack( const AString & include, bool & outCyclic )
{
    outCyclic = false;

    const int32_t stackSize = (int32_t)m_IncludeStack.GetSize();
    for ( int32_t i = ( stackSize - 1 ); i >= 0; --i )
    {
        AStackString<> possibleIncludePath( m_IncludeStack[ (size_t)i ]->m_FileName );
        const char * lastFwdSlash = possibleIncludePath.FindLast( '/' );
        const char * lastBackSlash = possibleIncludePath.FindLast( '\\' );
        const char * lastSlash = ( lastFwdSlash > lastBackSlash ) ? lastFwdSlash : lastBackSlash;
        ASSERT( lastSlash ); // it's a full path, so it must have a slash

        // truncate to slash (keep slash)
        possibleIncludePath.SetLength( (uint32_t)( lastSlash - possibleIncludePath.Get() ) + 1 );

        possibleIncludePath += include;

        NodeGraph::CleanPath( possibleIncludePath );

        // Handle cyclic includes
        const IncludedFile ** found = m_IncludeStack.FindDeref( possibleIncludePath );
        if ( found )
        {
            outCyclic = true;
            return *found;
        }

        const IncludedFile * file = FileExists( possibleIncludePath );
        ASSERT( file );
        if ( file->m_Exists )
        {
            return file;
        }

        // Try the next level of include stack
        // TODO: Could optimize out extra checks if path is the same
    }

    return nullptr; // not found
}

// ProcessIncludeFromIncludePath
//------------------------------------------------------------------------------
const IncludedFile * LightCache::ProcessIncludeFromIncludePath( const AString & include, bool & outCyclic )
{
    outCyclic = false;

    AStackString<> possibleIncludePath;
    for ( const AString & includePath : m_IncludePaths )
    {
        possibleIncludePath = includePath;
        possibleIncludePath += include;

        NodeGraph::CleanPath( possibleIncludePath );

        // Handle cyclic includes
        if ( m_IncludeStack.FindDeref( possibleIncludePath ) )
        {
            outCyclic = true;
            return nullptr;
        }

        const IncludedFile * file = FileExists( possibleIncludePath );
        ASSERT( file );
        if ( file->m_Exists )
        {
            return file;
        }

        // Try the next include path...
    }

    return nullptr; // not found
}

// FileExists
//------------------------------------------------------------------------------
const IncludedFile * LightCache::FileExists( const AString & fileName )
{
    const uint64_t fileNameHash = xxHash::Calc64( fileName );
    const uint64_t bucketIndex = fileNameHash % LIGHTCACHE_NUM_BUCKETS;
    IncludedFileBucket & bucket = g_AllIncludedFiles[ bucketIndex ];

    // Retrieve from shared cache
    {
        MutexHolder mh( bucket.m_Mutex );
        IncludedFile ** found = bucket.m_Files.FindDeref( fileName );
        if ( found )
        {
            return *found; // File previously handled so we can re-use the result
        }
    }

    // A newly seen file
    IncludedFile * newFile = FNEW( IncludedFile() );
    newFile->m_FileNameHash = fileNameHash;
    newFile->m_FileName = fileName;
    newFile->m_Exists = false;
    newFile->m_ContentHash = 0;

    // Try to open the new file
    FileStream f;
    if ( f.Open( fileName.Get() ) == false )
    {
        {
            // Store to shared cache
            MutexHolder mh( bucket.m_Mutex );
            bucket.m_Files.Append( newFile );
        }
        return newFile;
    }

    // File exists - parse it
    newFile->m_Exists = true;
    Parse( newFile, f );

    {
        // Store to shared cache
        MutexHolder mh( bucket.m_Mutex );
        bucket.m_Files.Append( newFile );
    }

    return newFile;
}

// SkipWhitepspace
//------------------------------------------------------------------------------
void LightCache::SkipWhitespace( const char * & pos ) const
{
    for (;;)
    {
        const char c = *pos;
        if ( ( c == ' ' ) || ( c == '\t' ) )
        {
            pos++;
            continue;
        }
        break;
    }
}

// IsAtEndOfLine
//------------------------------------------------------------------------------
bool LightCache::IsAtEndOfLine( const char * pos ) const
{
    const char c = *pos;
    return ( ( c == '\r' ) || ( c== '\n' ) );
}

// SkipLineEnd
//------------------------------------------------------------------------------
void LightCache::SkipLineEnd( const char * & pos ) const
{
    while ( IsAtEndOfLine( pos ) )
    {
        ++pos;
        continue;
    }
}

// SkipToEndOfLine
//------------------------------------------------------------------------------
void LightCache::SkipToEndOfLine( const char * & pos ) const
{
    for ( ;; )
    {
        const char c = *pos;
        if ( ( c != '\r' ) && ( c != '\n' ) && ( c != '\000' ) )
        {
            ++pos;
            continue;
        }
        break;
    }
}

// SkipToEndOfQuotedString
//------------------------------------------------------------------------------
bool LightCache::SkipToEndOfQuotedString( const char * & pos ) const
{
    // Skip opening char
    const char c = *pos;
    ASSERT( ( c == '"' ) || ( c == '<' ) );
    ++pos;

    // Determing expected end char
    const char endChar = ( c == '"' ) ? '"' : '>';

    // Find end char
    for ( ;; )
    {
        // Found it?
        if ( *pos == endChar )
        {
            ++pos;
            return true; // Found
        }

        // End of line?
        if ( ( *pos == '\r' ) || ( *pos == '\n' ) )
        {
            return false;
        }

        // Hit end of buffer?
        if ( *pos == 0 )
        {
            return false;
        }

        // Keep searching
        ++pos;
    }
}

//------------------------------------------------------------------------------
