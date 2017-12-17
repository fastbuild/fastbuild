// Function
//------------------------------------------------------------------------------
#pragma once

// includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"
#include "Tools/FBuild/FBuildCore/Error.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class BFFVariable;
class Dependencies;
class DirectoryListNode;
class Meta_AllowNonFile;
class Meta_File;
class Meta_Path;
class Node;
class ReflectionInfo;

// Function
//------------------------------------------------------------------------------
class Function
{
public:
    explicit    Function( const char * functionName );
    virtual     ~Function();

    // info about a function
    inline const AString & GetName() const { return m_Name; }

    // access to functions
    static const Function * Find( const AString & name );

    static void Create();
    static void Destroy();

    // does this function support a header? i.e. FunctionName( ...header.... )
    virtual bool AcceptsHeader() const; // can have a header
    virtual bool NeedsHeader() const;   // must have a header
    virtual bool NeedsBody() const;     // must have a body i.e. { ... }

    // must this function be unique?
    virtual bool IsUnique() const;
    inline bool GetSeen() const { return m_Seen; }
    inline void SetSeen() const { m_Seen = true; }

    // most functions don't need to override this
    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                const BFFIterator & functionNameStart,
                                const BFFIterator * functionBodyStartToken,
                                const BFFIterator * functionBodyStopToken,
                                const BFFIterator * functionHeaderStartToken,
                                const BFFIterator * functionHeaderStopToken ) const;

    // most functions will override this to commit the effects of the function
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const;

    // helpers to clean/fixup paths to files and folders
    static void CleanFolderPaths( Array< AString > & folders );
    static void CleanFilePaths( Array< AString > & files );
    void CleanFileNames( Array< AString > & fileNames ) const;

    bool GetDirectoryListNodeList( NodeGraph & nodeGraph,
                                   const BFFIterator & iter,
                                   const Array< AString > & paths,
                                   const Array< AString > & excludePaths,
                                   const Array< AString > & filesToExclude,
                                   const Array< AString > & excludePatterns,
                                   bool recurse,
                                   const Array< AString > * patterns,
                                   const char * inputVarName,
                                   Dependencies & nodes ) const;
    bool GetFileNode( NodeGraph & nodeGraph,
                      const BFFIterator & iter,
                      const AString & file,
                      const char * inputVarName,
                      Dependencies & nodes ) const;
    bool GetFileNodes( NodeGraph & nodeGraph,
                       const BFFIterator & iter,
                       const Array< AString > & files,
                       const char * inputVarName,
                       Dependencies & nodes ) const;
    bool GetObjectListNodes( NodeGraph & nodeGraph,
                             const BFFIterator & iter,
                             const Array< AString > & objectLists,
                             const char * inputVarName,
                             Dependencies & nodes ) const;

    bool GetNodeList( NodeGraph & nodeGraph, const BFFIterator & iter, const char * name, Dependencies & nodes, bool required = false,
                      bool allowCopyDirNodes = false, bool allowUnityNodes = false, bool allowRemoveDirNodes = false ) const;

    static bool GetNodeList( NodeGraph & nodeGraph,
                             const BFFIterator & iter,
                             const Function * function,
                             const char * propertyName,
                             const AString & nodeName,
                             Dependencies & nodes,
                             bool allowCopyDirNodes = false,
                             bool allowUnityNodes = false,
                             bool allowRemoveDirNodes = false );

    bool GetFileNode( NodeGraph & nodeGraph, const BFFIterator & iter, Node * & fileNode, const char * name, bool required = false ) const;

private:
    Function *  m_NextFunction;
    static Function * s_FirstFunction;

protected:
    AString     m_Name;
    mutable bool m_Seen; // track for unique enforcement

    // for functions that support a simple alias parameter, the base class can
    // parse it out
    mutable AString m_AliasForFunction;

    // helpers to get properties
    bool GetString( const BFFIterator & iter, const BFFVariable * & var, const char * name, bool required = false ) const;
    bool GetString( const BFFIterator & iter, AString & var, const char * name, bool required = false ) const;
    bool GetStringOrArrayOfStrings( const BFFIterator & iter, const BFFVariable * & var, const char * name, bool required ) const;
    bool GetBool( const BFFIterator & iter, bool & var, const char * name, bool defaultValue, bool required = false ) const;
    bool GetInt( const BFFIterator & iter, int32_t & var, const char * name, int32_t defaultValue, bool required ) const;
    bool GetInt( const BFFIterator & iter, int32_t & var, const char * name, int32_t defaultValue, bool required, int minVal, int maxVal ) const;
    bool GetStrings( const BFFIterator & iter, Array< AString > & strings, const char * name, bool required = false ) const;
    bool GetFolderPaths( const BFFIterator & iter, Array< AString > & strings, const char * name, bool required = false ) const;

    // helper function to make alias for target
    bool ProcessAlias( NodeGraph & nodeGraph, const BFFIterator & iter, Node * nodeToAlias ) const;
    bool ProcessAlias( NodeGraph & nodeGraph, const BFFIterator & iter, Dependencies & nodesToAlias ) const;

    // Reflection based property population
    bool GetNameForNode( NodeGraph & nodeGraph, const BFFIterator & iter, const ReflectionInfo * ri, AString & name ) const;
    bool PopulateProperties( NodeGraph & nodeGraph, const BFFIterator & iter, Node * node ) const;
    bool PopulateProperty( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateArrayOfStrings( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const;
    bool PopulateString( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const;
    bool PopulateBool( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateInt32( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateUInt32( const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateArrayOfStructs( NodeGraph & nodeGraph, const BFFIterator & iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;

    bool PopulateStringHelper( NodeGraph & nodeGraph, const BFFIterator & iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, Array< AString > & outStrings ) const;
    bool PopulateStringHelper( NodeGraph & nodeGraph, const BFFIterator & iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, const AString & string, Array< AString > & outStrings ) const;
    bool PopulatePathAndFileHelper( const BFFIterator & iter, const Meta_Path * pathMD, const Meta_File * fileMD, const AString & variableName, AString & valueToFix ) const;
};

//------------------------------------------------------------------------------
