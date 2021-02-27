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
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Mutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// System
#include <stdarg.h> // for va_start

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

    ~IncludedFile();

    uint64_t                        m_FileNameHash;
    AString                         m_FileName;
    bool                            m_Exists;
    uint64_t                        m_ContentHash;
    Array< Include >                m_Includes;
    Array< const IncludeDefine * >  m_IncludeDefines;
    Array< uint64_t >               m_NonIncludeDefines;

    inline bool operator == ( const AString & fileName ) const      { return ( m_FileName == fileName ); }
    inline bool operator == ( const IncludedFile & other ) const    { return ( ( m_FileNameHash == other.m_FileNameHash ) && ( m_FileName == other.m_FileName ) ); }
    inline bool operator <  ( const IncludedFile & other ) const    { return ( m_FileName < other.m_FileName ); }
};

// IncludedFileHashSet
//------------------------------------------------------------------------------
#define LIGHTCACHE_DEFAULT_BUCKET_SIZE 1024
class IncludedFileHashSet
{
public:
    IncludedFileHashSet( const IncludedFileHashSet & ) = delete;
    IncludedFileHashSet & operator=( const IncludedFileHashSet & ) = delete;
    IncludedFileHashSet()
    {
        m_Buckets.SetSize( LIGHTCACHE_DEFAULT_BUCKET_SIZE );
        for ( IncludedFile * & elt : m_Buckets )
        {
            elt = nullptr;
        }
    }

    ~IncludedFileHashSet()
    {
        Destruct();
    }

    const IncludedFile * Find( const AString & fileName, uint64_t fileNameHash )
    {
        const IncludedFile * const * location = InternalFind( fileName, fileNameHash );
        if ( location && *location )
        {
            return *location;
        }
        return nullptr;
    }

    // If two threads find the same include simultaneously, we delete the new
    // one and return the old one.
    const IncludedFile * Insert( IncludedFile * item )
    {
        if ( ( m_Buckets.GetSize() / 2 ) <= m_Elts )
        {
            const size_t newSize = ( m_Buckets.GetSize() < LIGHTCACHE_DEFAULT_BUCKET_SIZE )
                                 ? LIGHTCACHE_DEFAULT_BUCKET_SIZE
                                 : ( m_Buckets.GetSize() * 2 );
            Grow( newSize );
        }
        IncludedFile ** location = InternalFind( item->m_FileName, item->m_FileNameHash );
        ASSERT( location != nullptr );

        if ( *location != nullptr )
        {
            // A race between multiple threads got us a duplicate item.
            // delete the new one
            FDELETE item;
            return *location;
        }

        ++m_Elts;
        *location = item;
        return *location;
    }
    void Destruct()
    {
        for ( IncludedFile * file : m_Buckets )
        {
            FDELETE file;
        }
        m_Buckets.Destruct();
        m_Elts = 0;
    }

private:
    IncludedFile ** InternalFind( const AString & fileName, uint64_t fileNameHash )
    {
        if ( m_Buckets.IsEmpty() )
        {
            return nullptr;
        }
        size_t probeCount = 1;
        const size_t startIdx = fileNameHash & ( m_Buckets.GetSize() - 1 );
        IncludedFile ** bucket = &m_Buckets[ startIdx ];
        while ( *bucket != nullptr )
        {
            if ( ( (*bucket)->m_FileNameHash == fileNameHash ) && ( **bucket == fileName ) )
            {
                return bucket;
            }
            bucket = Next( m_Buckets, startIdx, probeCount );
        }
        ASSERT( *bucket == nullptr );
        return bucket;
    }

    static IncludedFile ** Next( Array< IncludedFile * > &buckets,
                                 size_t startIdx,
                                 size_t & probeCount )
    {
        size_t curIdx = startIdx + ( probeCount * probeCount ); //quadratic probing
        curIdx &= ( buckets.GetSize() - 1 );
        ++probeCount;
        return &buckets[ curIdx ];
    }
    void Grow( size_t elts )
    {
        Array< IncludedFile * > dest( elts, true );
        dest.SetSize( elts );
        for ( IncludedFile * & elt : dest )
        {
            elt = nullptr;
        }

        // populate dest with the elements in m_Buckets.  Rely on uniqueness
        // in source to avoid comparing in dest
        for ( IncludedFile * elt : m_Buckets )
        {
            if ( elt == nullptr )
            {
                continue;
            }
            size_t probeCount = 1;
            const size_t startIdx = elt->m_FileNameHash & ( dest.GetSize() - 1 );
            IncludedFile ** bucket = &dest[ startIdx ];
            while ( *bucket != nullptr )
            {
                bucket = Next( dest, startIdx, probeCount );
            }
            ASSERT( *bucket == nullptr );
            *bucket = elt;
        }
        m_Buckets.Swap( dest );
    }

    // m_Buckets must always be a size that is a power of 2
    Array< IncludedFile * > m_Buckets{ LIGHTCACHE_DEFAULT_BUCKET_SIZE, true };
    size_t m_Elts = 0;
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

// DESTRUCTOR
//------------------------------------------------------------------------------
IncludedFile::~IncludedFile()
{
    for ( const IncludeDefine * def : m_IncludeDefines )
    {
        FDELETE def;
    }
}

// IncludedFileBucket
//------------------------------------------------------------------------------
class IncludedFileBucket
{
public:
    IncludedFileBucket() = default;
    IncludedFileBucket( const IncludedFileBucket & ) = delete;
    IncludedFileBucket & operator = ( const IncludedFileBucket & ) = delete;

    void Destruct()
    {
        m_HashSet.Destruct();
    }
    Mutex                   m_Mutex;
    IncludedFileHashSet     m_HashSet;
};
// using a power of two number of buckets.  64 top level buckets should be a
// reasonable tradeoff between size and contention
#define LIGHTCACHE_NUM_BUCKET_BITS 6
#define LIGHTCACHE_NUM_BUCKETS ( 1ULL << LIGHTCACHE_NUM_BUCKET_BITS )
#define LIGHTCACHE_BUCKET_MASK_BASE ( LIGHTCACHE_NUM_BUCKETS - 1ULL )
// use upper bits for bucket selection, as lower bits get used in the hash set
#define LIGHTCACHE_HASH_TO_BUCKET(hash) ( (( hash ) >> ( 64ULL - LIGHTCACHE_NUM_BUCKET_BITS )) & LIGHTCACHE_BUCKET_MASK_BASE )
static IncludedFileBucket g_AllIncludedFiles[ LIGHTCACHE_NUM_BUCKETS ];

// CONSTRUCTOR
//------------------------------------------------------------------------------
LightCache::LightCache()
    : m_IncludePaths( 32, true )
    , m_AllIncludedFiles( 2048, true )
    , m_IncludeStack( 32, true )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LightCache::~LightCache() = default;

// Hash
//------------------------------------------------------------------------------
bool LightCache::Hash( ObjectNode * node,
                       const AString & compilerArgs,
                       uint64_t & outSourceHash,
                       Array< AString > & outIncludes )
{
    PROFILE_FUNCTION;

    StackArray<AString> forceIncludes;
    ProjectGeneratorBase::ExtractIncludePaths( compilerArgs,
                                               m_IncludePaths,
                                               forceIncludes,
                                               false ); // escapeQuotes

    // Ensure all includes are slash terminated
    for ( AString & includePath : m_IncludePaths )
    {
        if ( includePath.EndsWith( NATIVE_SLASH ) || includePath.EndsWith( OTHER_SLASH ) )
        {
            continue;
        }
        includePath += NATIVE_SLASH;
    }

    // Handle forced includes
    for ( AString & forceInclude : forceIncludes )
    {
        ProcessInclude( forceInclude, IncludeType::QUOTE );
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
    if ( m_Errors.IsEmpty() == false )
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
    const uint64_t fileSize = f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( (uint32_t)fileSize );
    if ( f.Read( fileContents.Get(), (size_t)fileSize ) != fileSize )
    {
        AddError( file, nullptr, "Error reading file: %s", LAST_ERROR_STR );
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
                ASSERT( m_Errors.IsEmpty() == false ); // ParseDirective reports error if encountered
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
    if ( ( *pos == '"' ) || ( *pos == '<' ) )
    {
        // Looks like a normal include
        IncludeType includeType;
        if ( ParseIncludeString( pos, include, includeType ) == false )
        {
            // We encountered an include we can't handle
            AddError( &file, pos, "Invalid or unsupported include." );
            return false;
        }

        file.m_Includes.EmplaceBack( include, includeType );
        return true;
    }

    // Not a normal include - perhaps this is a macro?
    AStackString<> macroName;
    if ( ParseMacroName( pos, macroName ) == false )
    {
        // We saw an unexpected sequence after the #include
        AddError( &file, pos, "Unexpected sequence after include." );
        return false;
    }

    // Store the macro include which will be resolved later
    file.m_Includes.EmplaceBack( macroName, IncludeType::MACRO );
    return true;
}

// ParseDirective_Define
//------------------------------------------------------------------------------
bool LightCache::ParseDirective_Define( IncludedFile & file, const char * & pos )
{
    // skip "include" and whitespace
    ASSERT( AString::StrNCmp( pos, "define", 6 ) == 0 );
    pos += 6;
    SkipWhitespace( pos );

    // Get macro name
    AStackString<> macroName;
    const char * macroStart = pos;
    if ( ParseMacroName( pos, macroName ) == false )
    {
        // Unexpected macro form - we don't know how to handle this
        AddError( &file, macroStart, "Unexpected macro form." );
        return false;
    }

    SkipWhitespace( pos );

    // Is this defining an include path?
    AStackString<> include;
    IncludeType includeType;
    if ( ParseIncludeString( pos, include, includeType ) == false )
    {
        // Not an include. We only care that this exists (not what it resolves to)
        file.m_NonIncludeDefines.Append( xxHash::Calc64( macroName ) );
        return true;
    }

    // Take note of the macro and the path it defines
    file.m_IncludeDefines.Append( FNEW( IncludeDefine( macroName, include, includeType ) ) );

    return true;
}

// ParseDirective_Import
//------------------------------------------------------------------------------
bool LightCache::ParseDirective_Import( IncludedFile & file, const char * & pos )
{
    // We encountered an import directive, we can't handle them.
    AddError( &file, pos, "#import is unsupported." );
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

            // Find macro - expand each possible value (we must
            // expand all values because we can't definitively know
            // which one is correct)
            // NOTE: Array can be extended/can move while being iterated,
            // so iterate by index only to original size
            const size_t originalSize = m_IncludeDefines.GetSize();
            for ( size_t i = 0; i < originalSize; ++i )
            {
                const IncludeDefine * def = m_IncludeDefines[ i ];
                if ( def->m_Macro == include )
                {
                    ProcessInclude( def->m_Include, def->m_Type );
                    return;
                }
            }

            // The macro was not defined as a valid include path
            // Is it defined at all?
            const uint64_t hash = xxHash::Calc64( include );
            bool foundNonIncludeDefine = false;
            for ( const IncludedFile * includedFile : m_AllIncludedFiles )
            {
                if ( includedFile->m_NonIncludeDefines.Find( hash ) != nullptr )
                {
                    foundNonIncludeDefine = true;
                    break;
                }
            }

            // If we didn't find the macro at all, we can assume that the include
            // is probably some variant of this:
            //
            // #if THING
            //     #include THING
            // #endif
            //
            // This means we can safely ignore this include as either:
            // a) The include is not used (guarded as above)
            // OR
            // b) The file is invalid and won't compile anyway
            //
            if ( foundNonIncludeDefine == false )
            {
                return;
            }

            // We found the macro, but since it's not an include path, this means
            // it is some complex structure, possibly referencing other macros
            // that we currently don't support.
            AddError( nullptr, nullptr, "Could not resolve macro '%s'.",
                                        include.Get() );
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
    const IncludedFile * const * found = m_IncludeStack.FindDeref( include );
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
        const IncludedFile * const * found = m_IncludeStack.FindDeref( possibleIncludePath );
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
    const uint64_t bucketIndex = LIGHTCACHE_HASH_TO_BUCKET( fileNameHash );
    IncludedFileBucket & bucket = g_AllIncludedFiles[ bucketIndex ];
    // Retrieve from shared cache
    {
        MutexHolder mh( bucket.m_Mutex );
        const IncludedFile * location = bucket.m_HashSet.Find( fileName, fileNameHash );
        if ( location )
        {
            m_IncludeDefines.Append( location->m_IncludeDefines );
            
            return location; // File previously handled so we can re-use the result
        }
    }

    // A newly seen file
    IncludedFile * newFile = FNEW( IncludedFile() );
    const IncludedFile * retval = nullptr;
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
            retval = bucket.m_HashSet.Insert( newFile );
        }
        return retval;
    }

    // File exists - parse it
    newFile->m_Exists = true;
    Parse( newFile, f );

    {
        // Store to shared cache
        MutexHolder mh( bucket.m_Mutex );
        retval = bucket.m_HashSet.Insert( newFile );
    }

    m_IncludeDefines.Append( retval->m_IncludeDefines );

    return retval;
}

// AddError
//------------------------------------------------------------------------------
void LightCache::AddError( IncludedFile * file,
                           const char * pos,
                           MSVC_SAL_PRINTF const char * formatString,
                           ... )
{
    // Format the error-specific output
    AStackString< 1024 > msgBuffer;
    va_list args;
    va_start( args, formatString );
    msgBuffer.VFormat( formatString, args );
    va_end( args );

    AStackString< 1024 > finalBuffer;
    finalBuffer.Format( "  Problem: %s\n", msgBuffer.Get() );

    // Annotate with file name
    if ( file )
    {
        finalBuffer.AppendFormat( "  File   : %s\n", file->m_FileName.Get() );
    }

    // Annotate with problem line
    if ( pos )
    {
        // Get the problem line
        AStackString<> line;
        ExtractLine( pos, line );
        finalBuffer.AppendFormat( "  Line   : %s\n", line.Get() );
    }

    // Append to list of errors
    m_Errors.Append( finalBuffer );
}

// SkipWhitepspace
//------------------------------------------------------------------------------
/*static*/ void LightCache::SkipWhitespace( const char * & pos )
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
/*static*/ bool LightCache::IsAtEndOfLine( const char * pos )
{
    const char c = *pos;
    return ( ( c == '\r' ) || ( c== '\n' ) );
}

// SkipLineEnd
//------------------------------------------------------------------------------
/*static*/ void LightCache::SkipLineEnd( const char * & pos )
{
    while ( IsAtEndOfLine( pos ) )
    {
        ++pos;
        continue;
    }
}

// SkipToEndOfLine
//------------------------------------------------------------------------------
/*static*/ void LightCache::SkipToEndOfLine( const char * & pos )
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
/*static*/ bool LightCache::SkipToEndOfQuotedString( const char * & pos )
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

// ExtractLine
//------------------------------------------------------------------------------
/*static*/ void LightCache::ExtractLine( const char * pos, AString & outLine )
{
    const char * start = pos;
    SkipToEndOfLine( pos );
    outLine.Assign( start, pos );
}

//------------------------------------------------------------------------------
