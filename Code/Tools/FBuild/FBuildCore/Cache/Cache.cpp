// Cache - Default cache implementation
//------------------------------------------------------------------------------

// Incldues
//------------------------------------------------------------------------------
#include "Cache.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Time.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// CacheStats
//------------------------------------------------------------------------------
class CacheStats
{
public:
    uint32_t    m_NumFiles = 0;
    uint64_t    m_NumBytes = 0;
};

// OldestFileTimeSorter
//------------------------------------------------------------------------------
class OldestFileTimeSorter
{
public:
    bool operator () ( const FileIO::FileInfo & a, const FileIO::FileInfo & b ) const
    {
        return ( a.m_LastWriteTime < b.m_LastWriteTime );
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
/*explicit*/ Cache::Cache() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ Cache::~Cache() = default;

// Init
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Init( const AString & cachePath,
                              const AString & cachePathMountPoint,
                              bool /*cacheRead*/,
                              bool /*cacheWrite*/,
                              bool /*cacheVerbose*/,
                              const AString & /*pluginDLLConfig*/ )
{
    PROFILE_FUNCTION;

    m_CachePath = cachePath;
    PathUtils::EnsureTrailingSlash( m_CachePath );

    // Check cache mount point if option is enabled
    #if defined( __WINDOWS__ )
        (void)cachePathMountPoint; // Not supported on Windows
    #else
        if ( cachePathMountPoint.IsEmpty() == false )
        {
            if ( FileIO::GetDirectoryIsMountPoint( cachePathMountPoint ) == false )
            {
                FLOG_WARN( "Caching disabled because '%s' is not a mount point", cachePathMountPoint.Get() );
                return false;
            }
        }
    #endif

    if ( FileIO::EnsurePathExists( m_CachePath ) )
    {
        return true;
    }

    FLOG_WARN( "Cache inaccessible - Caching disabled (Path '%s')", m_CachePath.Get() );
    return false;
}

// Shutdown
//------------------------------------------------------------------------------
/*virtual*/ void Cache::Shutdown()
{
    // Nothing to do
}

// Publish
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Publish( const AString & cacheId, const void * data, size_t dataSize )
{
    AStackString<> fullPath;
    GetFullPathForCacheEntry( cacheId, fullPath );

    // make sure the cache output path exists
    if ( !FileIO::EnsurePathExistsForFile( fullPath ) )
    {
        return false;
    }

    // open output cache (tmp) file
    AStackString<> fullPathTmp( fullPath );
    fullPathTmp += ".tmp";
    FileStream cacheTmpFile;
    if ( !cacheTmpFile.Open( fullPathTmp.Get(), FileStream::WRITE_ONLY ) )
    {
        return false;
    }

    // write data
    const bool cacheTmpWriteOk = ( cacheTmpFile.Write( data, dataSize ) == dataSize );
    cacheTmpFile.Close();

    if ( !cacheTmpWriteOk )
    {
        // failed to write to cache tmp file
        FileIO::FileDelete( fullPathTmp.Get() ); // try to cleanup failure
        return false;
    }

    // rename tmp file to real file
    if ( FileIO::FileMove( fullPathTmp, fullPath ) == false )
    {
        // try to delete (possibly) existing file
        FileIO::FileDelete( fullPath.Get() );

        // try rename again
        if ( FileIO::FileMove( fullPathTmp, fullPath ) == false )
        {
            // problem renaming file
            FileIO::FileDelete( fullPathTmp.Get() ); // try to cleanup tmp file
            return false;
        }
    }

    return true;
}

// Retrieve
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Retrieve( const AString & cacheId, void * & data, size_t & dataSize )
{
    data = nullptr;
    dataSize = 0;

    AStackString<> fullPath;
    GetFullPathForCacheEntry( cacheId, fullPath );

    FileStream cacheFile;
    if ( cacheFile.Open( fullPath.Get(), FileStream::READ_ONLY ) )
    {
        const size_t cacheFileSize = (size_t)cacheFile.GetFileSize();
        UniquePtr< char > mem( (char *)ALLOC( cacheFileSize ) );
        if ( cacheFile.Read( mem.Get(), cacheFileSize ) == cacheFileSize )
        {
            dataSize = cacheFileSize;
            data = mem.Release();
            return true;
        }
    }

    return false;
}

// FreeMemory
//------------------------------------------------------------------------------
/*virtual*/ void Cache::FreeMemory( void * data, size_t /*dataSize*/ )
{
    FREE( data );
}

// OutputInfo
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::OutputInfo( bool showProgress )
{
    // Count/Size per day
    const uint32_t NUM_DAYS( 30 );
    CacheStats perDay[ NUM_DAYS ];

    // Get all the files
    Array< FileIO::FileInfo > allFiles( 1000000 );
    uint64_t totalSize = 0;
    GetCacheFiles( showProgress, allFiles, totalSize );

    // Assign files into buckets
    const uint64_t currentTime = Time::GetCurrentFileTime(); // Compare filetimes to now
    for ( const FileIO::FileInfo & info : allFiles )
    {
        // Determine age bucket
        const uint64_t age = currentTime - info.m_LastWriteTime;
        #if defined( __WINDOWS__ )
            const uint64_t oneDay = ( 24 * 60 * 60 * (uint64_t)10000000 );
        #else
            const uint64_t oneDay = ( 24 * 60 * 60 * (uint64_t)1000000000 );
        #endif
        uint32_t ageInDays = (uint32_t)( age / oneDay );
        if ( ageInDays >= 30 )
        {
            ageInDays = 29;
        }
        perDay[ ageInDays ].m_NumFiles++;
        perDay[ ageInDays ].m_NumBytes += info.m_Size;
    }

    // Totals
    CacheStats total;
    total.m_NumBytes = totalSize;
    total.m_NumFiles = (uint32_t)allFiles.GetSize();

    // Generate cache info string
    OUTPUT( "================================================================================\n" );
    OUTPUT( " Age (Days) | Files    | Size (MiB) | %%\n" );
    OUTPUT( "================================================================================\n" );
    for ( uint32_t i=0; i<30; ++i )
    {
        const uint32_t num = perDay[ i ].m_NumFiles;
        const uint64_t size = perDay[ i ].m_NumBytes / MEGABYTE;
        const float sizePerc = ( total.m_NumBytes > 0 ) ? 100.0f * ( (float)size / (float)( total.m_NumBytes / MEGABYTE ) ) : 0.0f;
        AStackString<> graphBar;
        for ( uint32_t j=0; j < (uint32_t)(sizePerc); ++j )
        {
            if ( graphBar.GetLength() < 35 )
            {
                graphBar += '*';
            }
        }
        OUTPUT( " %2u%c        | %8u | %10" PRIu64 " | %5.1f %s\n", i, ( i == 29 ) ? '+' : ' ', num, size, (double)sizePerc, graphBar.Get() );
    }
    OUTPUT( "================================================================================\n" );
    OUTPUT( " Total      | %8u | %10" PRIu64 " |\n", total.m_NumFiles, total.m_NumBytes / MEGABYTE );
    OUTPUT( "================================================================================\n" );

    return true;
}

// Trim
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Trim( bool showProgress, uint32_t sizeMiB )
{
    // Get all the files
    Array< FileIO::FileInfo > allFiles( 1000000 );
    uint64_t totalSize = 0;
    GetCacheFiles( showProgress, allFiles, totalSize );
    OUTPUT( " - Before: %u Files @ %u MiB\n", (uint32_t)allFiles.GetSize(), (uint32_t)( totalSize / MEGABYTE ) );

    // Sort by age
    OldestFileTimeSorter sorter;
    allFiles.Sort( sorter );

    // Do we need to delete anything?
    OUTPUT( "Trimming to %u MiB:\n", sizeMiB );
    const uint64_t limit = ( (uint64_t)sizeMiB * MEGABYTE );
    uint32_t numDeleted = 0;
    if ( limit < totalSize )
    {
        const Timer timer;
        float lastProgressTime = 0.0f;
        if ( showProgress )
        {
            FLog::OutputProgress( 0.0f, 0.0f, 0, 0, 0, 0 );
        }
        const uint64_t originalTotalSize = totalSize;

        // Iterate over files, deleting oldest first
        for ( const FileIO::FileInfo & info : allFiles )
        {
            // Try to delete (ok to fail if file is in use)
            if ( FileIO::FileDelete( info.m_Name.Get() ) )
            {
                totalSize -= info.m_Size;
                ++numDeleted;

                // Are we under the limit now?
                if ( totalSize <= limit )
                {
                    break;
                }

                // Progress
                if ( showProgress )
                {
                    // Throttled to avoid perf impact
                    if ( ( timer.GetElapsed() - lastProgressTime ) > 0.5f )
                    {
                        const uint64_t toDeleteBytes = originalTotalSize - limit;
                        const uint64_t deletedBytes = originalTotalSize - totalSize;
                        const float perc = ( (float)deletedBytes / (float)toDeleteBytes ) * 100.0f;
                        FLog::OutputProgress( timer.GetElapsed(), perc, 0, 0, 0, 0 );
                        lastProgressTime = timer.GetElapsed();
                    }
                }
            }
        }

        if ( showProgress )
        {
            FLog::ClearProgress();
        }
    }

    OUTPUT( " - After: %u Files @ %u MiB\n", (uint32_t)allFiles.GetSize() - numDeleted, (uint32_t)( totalSize / MEGABYTE ) );
    return true;
}

// GetCacheFiles
//------------------------------------------------------------------------------
void Cache::GetCacheFiles( bool showProgress,
                           Array< FileIO::FileInfo > & outInfo,
                           uint64_t & outTotalSize ) const
{
    // Throttle progress messages to avoid impacting performance significantly
    // (network cache is usually the bottleneck, but a local caches is not)
    const Timer timer;
    float lastProgressTime = 0.0f;
    if ( showProgress )
    {
        FLog::OutputProgress( 0.0f, 0.0f, 0, 0, 0, 0 );
    }

    // Scan the matrix of directories
    for ( size_t i=0; i<256; ++i )
    {
        for ( size_t j=0; j<256; ++j )
        {
            // Get Files
            AStackString<> path;
            path.Format( "%s%02X%c%02X%c", m_CachePath.Get(),
                                               (uint32_t)i,
                                               NATIVE_SLASH,
                                               (uint32_t)j,
                                               NATIVE_SLASH);
            FileIO::GetFilesEx( path, nullptr, false, &outInfo );

            // Progress
            if ( showProgress )
            {
                // Throttled to avoid perf impact
                if ( ( timer.GetElapsed() - lastProgressTime ) > 0.5f )
                {
                    const float perc = ( (float)( ( i*256 ) + j ) / (float)( 256 * 256 ) ) * 100.0f;
                    FLog::OutputProgress( timer.GetElapsed(), perc, 0, 0, 0, 0 );
                    lastProgressTime = timer.GetElapsed();
                }
            }
        }
    }

    // Calculate totals
    outTotalSize = 0;
    for ( const FileIO::FileInfo & info : outInfo )
    {
        outTotalSize += info.m_Size;
    }

    if ( showProgress )
    {
        FLog::ClearProgress();
    }
}

// GetFullPathForCacheEntry
//------------------------------------------------------------------------------
void Cache::GetFullPathForCacheEntry( const AString & cacheId,
                                      AString & outFullPath ) const
{
    // format example: N:\\fbuild.cache\\AA\\BB\\<ABCD.......>
    outFullPath.Format( "%s%c%c%c%c%c%c%s", m_CachePath.Get(),
                                            cacheId[ 0 ],
                                            cacheId[ 1 ],
                                            NATIVE_SLASH,
                                            cacheId[ 2 ],
                                            cacheId[ 3 ],
                                            NATIVE_SLASH,
                                            cacheId.Get() );
}

//------------------------------------------------------------------------------
