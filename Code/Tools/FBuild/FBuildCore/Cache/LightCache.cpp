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

// IncludedFile
//------------------------------------------------------------------------------
class IncludedFile
{
public:
    class Include
    {
    public:
        Include( const AString & include, bool angleBracketForm )
            : m_Include( include )
            , m_AngleBracketForm( angleBracketForm )
        {}

        AString                     m_Include;
        bool                        m_AngleBracketForm;
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

#define LIGHTCACHE_DEFAULT_BUCKET_SIZE 1024
class IncludedFileHashSet
{
public:
    IncludedFileHashSet( const IncludedFileHashSet & ) = delete;
    IncludedFileHashSet &operator=( const IncludedFileHashSet & ) = delete;
    ~IncludedFileHashSet()
    {
        Destruct();
    }
    IncludedFileHashSet()
    {
        m_Buckets.SetSize( LIGHTCACHE_DEFAULT_BUCKET_SIZE );
        for( auto &elt : m_Buckets )
        {
            elt = nullptr;
        }
    }
    IncludedFile **Find( const AString & fileName, uint64_t fileNameHash )
    {
        if( m_Buckets.IsEmpty() )
           return nullptr;
        size_t probe_count = 1;
        size_t startIdx = fileNameHash & ( m_Buckets.GetSize() - 1 );
        IncludedFile **bucket = &m_Buckets[ startIdx ];
        while( *bucket != nullptr )
        {
            if( (*bucket)->m_FileNameHash == fileNameHash && **bucket == fileName )
            {
                return bucket;
            }
            bucket = Next( m_Buckets, startIdx, probe_count );
        }
        return bucket;
    }
    void Insert( IncludedFile *item, IncludedFile **location )
    {
        if( m_Buckets.GetSize() / 2 <= m_Elts )
        {
            size_t newSize = (
               m_Buckets.GetSize() < LIGHTCACHE_DEFAULT_BUCKET_SIZE) ?
                  LIGHTCACHE_DEFAULT_BUCKET_SIZE :
                  ( m_Buckets.GetSize() * 2 );
            Grow( newSize );
            location = nullptr;
        }
        if( location == nullptr || location < m_Buckets.begin() || location >= m_Buckets.end() )
        {
            // passed in location is now invalid, re-find our item.
            // we can assume it isn't present
            size_t probe_count = 1;
            size_t startIdx = item->m_FileNameHash & ( m_Buckets.GetSize() - 1 );
            location = &m_Buckets[ startIdx ];
            while( *location != nullptr )
            {
                location = Next( m_Buckets, startIdx, probe_count );
            }
        }
        ++m_Elts;
        *location = item;
    }
    void Destruct()
    {
        for ( IncludedFile * file : m_Buckets )
        {
            FDELETE file;
        }
        m_Buckets.Destruct();
    }

private:
    static IncludedFile **Next( Array< IncludedFile * > &buckets,
                                  size_t startIdx,
                                  size_t &probe_count)
    {
        size_t curIdx = startIdx + probe_count * probe_count; //quadratic probing
        curIdx &= ( buckets.GetSize() - 1 );
        ++probe_count;
        return &buckets[ curIdx ];
    }
    void Grow( size_t elts )
    {
        Array< IncludedFile * > dest{ elts, true };
        dest.SetSize( elts );
        for( auto &elt : dest )
        {
            elt = nullptr;
        }

        // populate dest with the elements in m_Buckets.  Rely on uniqueness
        // in source to avoid comparing in dest
        for( IncludedFile *elt : m_Buckets )
        {
            if( elt == nullptr )
               continue;
            size_t probe_count = 1;
            size_t startIdx = elt->m_FileNameHash & ( dest.GetSize() - 1 );
            IncludedFile **bucket = &dest[ startIdx ];
            while( *bucket != nullptr )
            {
                bucket = Next( dest, startIdx, probe_count );
            }
            *bucket = elt;
        }
        m_Buckets.Swap( dest );
    }

    // m_Buckets must always be a size that is a power of 2
    Array< IncludedFile * > m_Buckets{ 1024, true };
    size_t m_Elts = 0;
};

// IncludedFileBucket
//------------------------------------------------------------------------------
class IncludedFileBucket
{
public:
    IncludedFileBucket()
        : m_Mutex()
    {}
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
#define LIGHTCACHE_NUM_BUCKETS ( 1ULL<<6 )
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
    , m_ProblemParsing( false )
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
    const IncludedFile * includedFile = ProcessInclude( rootFileName, false );

    // Handle missing root file
    if ( includedFile == nullptr )
    {
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

// Hash
//------------------------------------------------------------------------------
void LightCache::Hash( IncludedFile * file, FileStream & f )
{
    ASSERT( f.IsOpen() );

    // Read all contents
    const uint64_t fileSize = f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( (uint32_t)fileSize );
    if ( f.Read( fileContents.Get(), (size_t)fileSize ) != fileSize )
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

        // line contains something useful
        // is it an include directive?
        const char c = *pos;
        if ( c == '#' )
        {
            pos++;
            SkipWhitespace( pos );
            if ( AString::StrNCmp( pos, "include", 7 ) == 0 )
            {
                pos += 7;
                SkipWhitespace( pos );
                if ( ( *pos != '"' ) && ( *pos != '<' ) )
                {
                    // We encountered an include we can't handle (using a macro for the path for example)
                    m_ProblemParsing = true;
                    return;
                }
                const bool angleBracketForm = ( *pos == '<' );
                ++pos;
                const char * includeStart = pos;
                SkipToEndOfQuotedString( pos );
                const char * includeEnd = pos;
                AStackString<> include( includeStart, includeEnd );

                file->m_Includes.Append( IncludedFile::Include{ include, angleBracketForm } );

                SkipToEndOfLine( pos );
                SkipLineEnd( pos );
                continue;
            }
            else if ( AString::StrNCmp( pos, "import", 6 ) == 0 )
            {
                // We encountered an import directive, we can't handle them.
                m_ProblemParsing = true;
                return;
            }
        }

        // block comments
        if ( ( c == '/' ) && ( pos[ 1 ] == '*' ) )
        {
            pos += 2; // skip /*
            for (;;)
            {
                const char thisChar = *pos;
                // end of data?
                if ( thisChar == 0 )
                {
                    break;
                }
                // enf of comment block?
                if ( ( thisChar == '*' ) && ( pos[ 1 ] == '/' ) )
                {
                    pos +=2;
                    break;
                }

                // line ending - windows
                if ( c == '\r' )
                {
                    ++pos;
                    if ( *pos == '\n' )
                    {
                        ++pos;
                    }
                    continue;
                }

                // line ending - unix
                if ( c == '\n' )
                {
                    ++pos;
                    continue;
                }

                // part of comment
                ++pos;
            }
        }

        // a non-include line
        SkipToEndOfLine( pos );
        SkipLineEnd( pos );
    }
}

// ProcessInclude
//------------------------------------------------------------------------------
const IncludedFile * LightCache::ProcessInclude( const AString & include, bool angleBracketForm )
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
        // From MSDN: http://msdn.microsoft.com/en-us/library/36k2cdd4.aspx

        if ( angleBracketForm )
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
        else
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

    if ( file )
    {
        // Avoid recursing into files we've already seen
        if ( cyclic )
        {
            return file;
        }

        // Have we already seen this file before?
        if ( m_AllIncludedFiles.FindDeref( *file ) )
        {
            return file;
        }

        // Take note of this included file
        m_AllIncludedFiles.Append( file );

        // Recurse
        m_IncludeStack.Append( file );
        for ( const IncludedFile::Include & inc : file->m_Includes )
        {
            ProcessInclude( inc.m_Include, inc.m_AngleBracketForm );
        }
        m_IncludeStack.Pop();

        return file;
    }

    // Include not found. This is ok because:
    // a) The file might not be needed. If the include is within an inactive part of the file
    //    such as a comment or ifdef'd for example. If compilation succeeds, the file should
    //    not be part of our dependencies anyway, so this is ok.
    // b) The files is genuinely missing, in which case compilation will fail. If compilation
    //    fails then we won't bake the dependencies with the missing file, so this is ok.
    return nullptr;
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
    const uint64_t bucketIndex = LIGHTCACHE_HASH_TO_BUCKET( fileNameHash );
    IncludedFileBucket & bucket = g_AllIncludedFiles[ bucketIndex ];
    IncludedFile ** location = nullptr;
    // Retrieve from shared cache
    {
        MutexHolder mh( bucket.m_Mutex );
        location = bucket.m_HashSet.Find( fileName, fileNameHash );
        if ( location && *location )
        {
            return *location; // File previously handled so we can re-use the result
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
            bucket.m_HashSet.Insert( newFile, location );
        }
        return newFile;
    }

    // File exists - parse it
    newFile->m_Exists = true;
    Hash( newFile, f );

    {
        // Store to shared cache
        MutexHolder mh( bucket.m_Mutex );
        bucket.m_HashSet.Insert( newFile, location );
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
void LightCache::SkipToEndOfQuotedString( const char * & pos ) const
{
    for ( ;; )
    {
        const char c = *pos;
        if ( ( c != '"' ) && ( c != '>' ) )
        {
            ++pos;
            continue;
        }
        break;
    }
}

//------------------------------------------------------------------------------
