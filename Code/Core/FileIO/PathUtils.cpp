// PathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "PathUtils.h"
#include "Core/Strings/AStackString.h"
#include <string.h>

// Exists
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFolderPath( const AString & path )
{
    const size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];

        // Handle both slash types so we cope with non-cleaned paths
        if ( ( lastChar == NATIVE_SLASH ) || ( lastChar == OTHER_SLASH ) )
        {
            return true;
        }
    }
    return false;
}

// IsFullPath
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFullPath( const AString & path )
{
    #if defined( __WINDOWS__ )
        // full paths on Windows have a drive letter and colon, or are unc
        return ( ( path.GetLength() >= 2 && path[ 1 ] == ':' ) ||
                 path.BeginsWith( NATIVE_DOUBLE_SLASH ) );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // full paths on Linux/OSX/IOS begin with a slash
        return path.BeginsWith( NATIVE_SLASH );
    #endif
}

// ArePathsEqual
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::ArePathsEqual( const AString & cleanPathA, const AString & cleanPathB )
{
    #if defined( __LINUX__ )
        // Case Sensitive
        return ( cleanPathA == cleanPathB );
    #endif

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Case Insensitive
        return ( cleanPathA.CompareI( cleanPathB ) == 0 );
    #endif
}

// IsWildcardMatch
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsWildcardMatch( const char * pattern, const char * path )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return AString::Match( pattern, path );
    #else
        // Windows & OSX : Case insensitive
        return AString::MatchI( pattern, path );
    #endif
}

// PathBeginsWith
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathBeginsWith( const AString & cleanPath, const AString & cleanSubPath )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return cleanPath.BeginsWith( cleanSubPath );
    #else
        // Windows & OSX : Case insensitive
        return cleanPath.BeginsWithI( cleanSubPath );
    #endif
}

// PathEndsWithFile
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathEndsWithFile( const AString & cleanPath, const AString & fileName )
{
    // Work out if ends match
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        const bool endMatch = cleanPath.EndsWith( fileName );
    #else
        // Windows & OSX : Case insensitive
        const bool endMatch = cleanPath.EndsWithI( fileName );
    #endif
    if ( !endMatch )
    {
        return false;
    }

    // if it's an entire match (a full path for example)
    if ( cleanPath.GetLength() == fileName.GetLength() )
    {
        return true;
    }

    // Sanity check - if fileName was longer then path (or equal) we can't get here
    ASSERT( cleanPath.GetLength() > fileName.GetLength() );
    const size_t potentialSlashIndex = ( cleanPath.GetLength() - fileName.GetLength() ) - 1; // -1 for char before match
    const char potentialSlash = cleanPath[ potentialSlashIndex ];
    if ( potentialSlash == NATIVE_SLASH )
    {
        return true; // full filename part matches (e.g. c:\thing\stuff.cpp | stuff.cpp )
    }
    return false; // fileName is only a partial match (e.g. c:\thing\otherstuff.cpp | stuff.cpp )
}

// EnsureTrailingSlash
//------------------------------------------------------------------------------
/*static*/ void PathUtils::EnsureTrailingSlash( AString & path )
{
    // check for existing slash
    const size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];
        if ( lastChar == NATIVE_SLASH )
        {
            return; // good slash - nothing to do
        }
        if ( lastChar == OTHER_SLASH )
        {
            // bad slash, do fixup
            path[ pathLen - 1 ] = NATIVE_SLASH;
            return;
        }
    }

    // add slash
    path += NATIVE_SLASH;
}

// FixupFolderPath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFolderPath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    #if defined( __WINDOWS__ )
        const bool isUNCPath = path.BeginsWith( NATIVE_DOUBLE_SLASH );
    #endif
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    #if defined( __WINDOWS__ )
        if ( isUNCPath )
        {
            AStackString<> copy( path );
            path.Clear();
            path += NATIVE_SLASH; // Restore double slash by adding one back
            path += copy;
        }
    #endif

    // ensure slash termination
    if ( path.EndsWith( NATIVE_SLASH ) == false )
    {
        path += NATIVE_SLASH;
    }
}

// FixupFilePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFilePath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    // Sanity check - calling this function on a folder path is an error
    ASSERT( path.EndsWith( NATIVE_SLASH ) == false );
}

// StripFileExtension
//------------------------------------------------------------------------------
/*static*/ void PathUtils::StripFileExtension( AString & filePath )
{
    const char * lastDot = filePath.FindLast( '.' );
    if ( lastDot )
    {
        filePath.SetLength( (uint32_t)( lastDot - filePath.Get() ) );
    }
}

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetRelativePath( const AString & basePath,
                                            const AString & fileName,
                                            AString & outRelativeFileName )
{
    // Makes no sense to call with empty basePath
    ASSERT( basePath.IsEmpty() == false );

    // Can only determine relative paths if both are of the same scope
    ASSERT( IsFullPath( basePath ) == IsFullPath( fileName ) );

    // Handle base paths which are not slash terminated
    if ( basePath.EndsWith( NATIVE_SLASH ) == false )
    {
        AStackString<> basePathCopy( basePath );
        basePathCopy += NATIVE_SLASH;
        GetRelativePath( basePathCopy, fileName, outRelativeFileName );
        return;
    }

    // Find common sub-path
    const char * pathA = basePath.Get();
    const char * pathB = fileName.Get();
    const char * itA = pathA;
    const char * itB = pathB;
    char compA = *itA;
    char compB = *itB;

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Windows & OSX: Case insensitive
        if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
        {
            compA = 'a' + ( compA - 'A' );
        }
        if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
        {
            compB = 'a' + ( compB - 'A' );
        }
    #endif

    while ( ( compA == compB ) && ( compA != '\0' ) )
    {
        const bool dirToken = ( ( *itA == '/' ) || ( *itA == '\\' ) );
        itA++;
        compA = *itA;
        itB++;
        compB = *itB;
        if ( dirToken )
        {
            pathA = itA;
            pathB = itB;
        }

        #if defined( __WINDOWS__ ) || defined( __OSX__ )
            // Windows & OSX: Case insensitive
            if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
            {
                compA = 'a' + ( compA - 'A' );
            }
            if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
            {
                compB = 'a' + ( compB - 'A' );
            }
        #endif
    }

    const bool hasCommonSubPath = ( pathA != basePath.Get() );
    if ( hasCommonSubPath == false )
    {
        // No common sub-path, so use fileName as-is
        outRelativeFileName = fileName;
        return;
    }

    // Build relative path

    // For every remaining dir in the project path, go up one directory
    outRelativeFileName.Clear();
    for ( ;; )
    {
        const char c = *pathA;
        if ( c == 0 )
        {
            break;
        }
        if ( ( c == '/' ) || ( c == '\\' ) )
        {
            #if defined( __WINDOWS__ )
                outRelativeFileName += "..\\";
            #else
                outRelativeFileName += "../";
            #endif
        }
        ++pathA;
    }

    // Add remainder of source path relative to the common sub path
    outRelativeFileName += pathB;
}

// GetBaseName
//------------------------------------------------------------------------------

/* static */ void PathUtils::GetBaseName( const AString& pathName, AString& baseName )
{
    if ( const char * slashPos = pathName.FindLast( NATIVE_SLASH ) )
    {
        baseName = slashPos + 1;
    }
    else
    {
        baseName = pathName;
    }

}

// GetDirectoryName
//------------------------------------------------------------------------------

/* static */ void PathUtils::GetDirectoryName( const AString& pathName, AString& dirName )
{
    if ( pathName.EndsWith( NATIVE_SLASH ) )
    {
        dirName = pathName;
        return;
    }
    if ( const char * slashPos = pathName.FindLast( NATIVE_SLASH ) )
    {
        dirName.Assign( pathName.Get(), slashPos + 1 ); // Include slash
    }
    else
    {
        dirName.Clear();
    }
}

// JoinPath
//------------------------------------------------------------------------------

/* static */ void PathUtils::JoinPath( AString& pathName, const AString& baseName ) {
    EnsureTrailingSlash( pathName );
    pathName += baseName;
}


// One of the major problem we have to overcome for Distributed ThinLTO project is that path names on the local machine
// that serve as module ids and that are hardcoded in the summary index files do not match remote directory file path names.
// FASTBuild doesn't run compilation jobs on a virtual file system (like chroot or docker), instead it is using a real file
// system and in general case we cannot map file paths from the local file system to the remote file system.

// Luckily, almost always for Distributed ThinLTO path names are relative
// to the project directory. The idea is, that if all the files have one common ancestor node in the local file system and
// all the file paths are relative, then we can create a sub directory on the remote machine that
// would mimic the location of the local files. In this case, the module ids for Distributed ThinLTO
// will stay the same.
//
// The purpose of the code below is to implement a simpified virtual file system to write there all the paths that participate
// in the DTLTO whenever possible (e.g. it cannot be done if we have absolute paths).
namespace dtlto
{

// CompareFileName
//------------------------------------------------------------------------------
// Compares two file names (C-style strings) in a case-insensitive manner on Windows
// and case-sensitive manner on Linux.
static int CompareFileName( const char * nameA,
                            const char * nameB )
{
#ifdef __WINDOWS__
    return ::_stricmp( nameA, nameB );
#endif
#ifdef __LINUX__
    return ::strcmp( nameA, nameB );
#endif
}

// CompareFileName
//------------------------------------------------------------------------------
// Compares two file names (AString objects) by converting them to C-style strings
// and using the appropriate comparison function based on the operating system.
static int CompareFileName( const AString & nameA,
                            const AString & nameB )
{
    return CompareFileName( nameA.Get(), nameB.Get() );
}

struct DirectoryNode;

// FileNode
//------------------------------------------------------------------------------
// Represents a file node in a directory structure. Contains information about
// the file's name, and a pointer to its parent directory node.
struct FileNode
{
protected:
    DirectoryNode * parent = nullptr; // Pointer to the parent directory node
    AString name;                     // Name of the file

public:
    // Default constructor
    FileNode() = default;

    // Initializes the file node with the given name.
    FileNode( const AString & file_name )
        : name( file_name )
    {}

    // GetParent
    //------------------------------------------------------------------------------
    // Returns a pointer to the parent directory node.
    DirectoryNode * GetParent() const
    {
        return parent;
    }

    // SetParent
    //------------------------------------------------------------------------------
    // Sets the parent directory node.
    void SetParent( DirectoryNode * parent_ptr )
    {
        parent = parent_ptr;
    }

    // GetName
    //------------------------------------------------------------------------------
    // Returns the name of the file.
    const AString & GetName() const
    {
        return name;
    }

    // CompareName
    //------------------------------------------------------------------------------
    // Compares the name of this file node with another file name.
    int CompareName( const AString & file_name ) const
    {
        return CompareFileName( name, file_name );
    }
};


// DirectoryNode
//------------------------------------------------------------------------------
// Represents a directory node in a directory structure. Contains information about
// the directory's name, parent directory, child directories, and files.
struct DirectoryNode
{
protected:
    DirectoryNode * parent = nullptr; // Pointer to the parent directory node
    Array< DirectoryNode * > childs;  // Array of child directory nodes
    AString name;                     // Name of the directory
    Array< FileNode > files;          // Array of file nodes in the directory

public:
    // Default constructor
    DirectoryNode() = default;

    // Initializes the directory node with the given name.
    DirectoryNode( const AString & dir_name )
        : parent( nullptr ),
          name( dir_name )
    {}

    // Deletes all child directory nodes and clears the name, files, and childs arrays.
    virtual ~DirectoryNode()
    {
        for ( DirectoryNode * dir : GetChilds() )
        {
            if ( dir )
                delete dir;
        }
        name.Clear();
        files.Clear();
        childs.Clear();
    }

    // CompareName
    //------------------------------------------------------------------------------
    // Compares the name of this directory node with another directory name using the
    // CompareFileName function.
    int CompareName( const AString & dir_name ) const
    {
        return CompareFileName( name, dir_name );
    }

    // GetName
    //------------------------------------------------------------------------------
    // Returns the name of the directory.
    const AString & GetName() const
    {
        return name;
    }

    // GetParent
    //------------------------------------------------------------------------------
    // Returns a pointer to the parent directory node (const version).
    const DirectoryNode * GetParent() const
    {
        return parent;
    }

    // SetParent
    //------------------------------------------------------------------------------
    // Sets the parent directory node.
    void SetParent( DirectoryNode * parent_ptr )
    {
        parent = parent_ptr;
    }

    // GetFiles
    //------------------------------------------------------------------------------
    // Returns a reference to the array of file nodes in the directory (const version).
    const Array< FileNode > & GetFiles() const
    {
        return files;
    }

    // GetParent
    //------------------------------------------------------------------------------
    // Returns a pointer to the parent directory node.
    DirectoryNode * GetParent()
    {
        return parent;
    }

    // GetChilds
    //------------------------------------------------------------------------------
    // Returns a reference to the array of child directory nodes.
    Array< DirectoryNode * > & GetChilds()
    {
        return childs;
    }

    // GetFiles
    //------------------------------------------------------------------------------
    // Returns a reference to the array of file nodes in the directory.
    Array< FileNode > & GetFiles()
    {
        return files;
    }

    // FindFile
    //------------------------------------------------------------------------------
    // Finds a file node in the directory by its name. Returns a pointer to the file
    // node if found, otherwise returns nullptr.
    FileNode * FindFile( const AString & file_name )
    {
        for ( FileNode & file : GetFiles() )
        {
            if ( 0 == file.CompareName( file_name ) )
            {
                return &file;
            }
        }
        return nullptr;
    }

    FileNode * AddFile( const AString & file_name )
    {
        FileNode * file_node;
        if ( nullptr != ( file_node = FindFile( file_name ) ) )
            return file_node;
        FileNode fileNode( file_name);
        fileNode.SetParent( this );
        GetFiles().Append( fileNode );
        return FindFile( file_name );
    }

    // FindChildDirectory
    //------------------------------------------------------------------------------
    // Finds a child directory node by its name. Returns a pointer to the child directory
    // node if found, otherwise returns nullptr.
    DirectoryNode * FindChildDirectory( const AString & dir_name )
    {
        for ( DirectoryNode * dir : GetChilds() )
        {
            if ( 0 == dir->CompareName( dir_name ) )
                return dir;
        }
        return nullptr;
    }

    // Create
    //------------------------------------------------------------------------------
    // Creates a new directory node with the given name and sets its parent
    // directory. Returns a pointer to the created directory node.
    static DirectoryNode * Create( const AString & node_name,
                                   DirectoryNode * parent_node = nullptr )
    {
        DirectoryNode * dir_node = new DirectoryNode( node_name );
        if ( !dir_node )
            return nullptr;
        dir_node->SetParent( parent_node );
        return dir_node;
    }

    // Create
    //------------------------------------------------------------------------------
    // Creates a new directory node with the given name (C-style string) and
    // sets its parent directory. Returns a pointer to the created directory node.
    static DirectoryNode * Create( const char * name,
                                   DirectoryNode * parent_node = nullptr )
    {
        AString node_name( name );
        return Create( node_name, parent_node );
    }

    // AddChildDirectory
    //------------------------------------------------------------------------------
    // Adds a child directory node to the directory. If a child directory with the same
    // name already exists, returns a pointer to the existing child directory node.
    // Otherwise, creates a new child directory node, sets its parent to this directory,
    // and appends it to the childs array.
    DirectoryNode * AddChildDirectory( const AString & dir_name )
    {
        DirectoryNode * child_dir = nullptr;
        if ( nullptr != ( child_dir = FindChildDirectory( dir_name ) ) )
            return child_dir;
        child_dir = Create( dir_name, this );
        if ( !child_dir )
            return nullptr;
        GetChilds().Append( child_dir );
        return child_dir;
    }
};
}
