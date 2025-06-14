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
class BFFParser;
class BFFToken;
class BFFTokenRange;
class BFFVariable;
class Dependencies;
class DirectoryListNode;
class Meta_AllowNonFile;
class Meta_File;
class Meta_Path;
class Node;
class ReflectionInfo;
class CompilerNode;

// GetNodeListOptions
//------------------------------------------------------------------------------
class GetNodeListOptions
{
public:
    bool m_AllowCopyDirNodes = false;
    bool m_AllowUnityNodes = false;
    bool m_AllowRemoveDirNodes = false;
    bool m_AllowCompilerNodes = false;
};

// Function
//------------------------------------------------------------------------------
class Function
{
public:
    explicit Function( const char * functionName );
    virtual ~Function();

    // info about a function
    const AString & GetName() const { return m_Name; }

    // access to functions
    static const Function * Find( const AString & name );

    static void Create();
    static void Destroy();

    // does this function support a header? i.e. FunctionName( ...header.... )
    virtual bool AcceptsHeader() const; // can have a header
    virtual bool NeedsHeader() const;   // must have a header
    virtual bool NeedsBody() const;     // must have a body i.e. { ... }

    virtual Node * CreateNode() const;

    // must this function be unique?
    virtual bool IsUnique() const;
    bool GetSeen() const { return m_Seen; }
    void SetSeen() const { m_Seen = true; }

    // most functions don't need to override this
    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                BFFParser & parser,
                                const BFFToken * functionNameStart,
                                const BFFTokenRange & headerRange,
                                const BFFTokenRange & bodyRange ) const;

    // most functions will override this to commit the effects of the function
    virtual bool Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const;

    // Node::Initialize helpers
    static bool GetCompilerNode( NodeGraph & nodeGraph,
                                 const BFFToken * iter,
                                 const Function * function,
                                 const AString & compiler,
                                 CompilerNode *& compilerNode );
    static bool GetDirectoryListNodeList( NodeGraph & nodeGraph,
                                          const BFFToken * iter,
                                          const Function * function,
                                          const Array<AString> & paths,
                                          const Array<AString> & excludePaths,
                                          const Array<AString> & filesToExclude,
                                          const Array<AString> & excludePatterns,
                                          bool recurse,
                                          bool includeReadOnlyStatusInHash,
                                          bool includeDirs,
                                          const Array<AString> * patterns,
                                          const char * inputVarName,
                                          Dependencies & nodes );
    static bool GetFileNode( NodeGraph & nodeGraph,
                             const BFFToken * iter,
                             const Function * function,
                             const AString & file,
                             const char * inputVarName,
                             Dependencies & nodes );
    static bool GetFileNodes( NodeGraph & nodeGraph,
                              const BFFToken * iter,
                              const Function * function,
                              const Array<AString> & files,
                              const char * inputVarName,
                              Dependencies & nodes );
    static bool GetObjectListNodes( NodeGraph & nodeGraph,
                                    const BFFToken * iter,
                                    const Function * function,
                                    const Array<AString> & objectLists,
                                    const char * inputVarName,
                                    Dependencies & nodes );
    static bool GetNodeList( NodeGraph & nodeGraph,
                             const BFFToken * iter,
                             const Function * function,
                             const char * propertyName,
                             const Array<AString> & nodeNames,
                             Dependencies & nodes,
                             const GetNodeListOptions & options = GetNodeListOptions() );
    static bool GetNodeList( NodeGraph & nodeGraph,
                             const BFFToken * iter,
                             const Function * function,
                             const char * propertyName,
                             const AString & nodeName,
                             Dependencies & nodes,
                             const GetNodeListOptions & options = GetNodeListOptions() );
    static bool GetNodeList( const BFFToken * iter,
                             const Function * function,
                             const char * propertyName,
                             Node * node,
                             Dependencies & nodes,
                             const GetNodeListOptions & options = GetNodeListOptions() );

protected:
    AString m_Name;
    mutable bool m_Seen; // track for unique enforcement

    // for functions that support a simple alias parameter, the base class can
    // parse it out
    mutable AString m_AliasForFunction;
    mutable const BFFToken * m_AliasForFunctionSourceToken = nullptr;

    // Helpers to get nodes
    bool GetNodeList( NodeGraph & nodeGraph,
                      const BFFToken * iter,
                      const char * name,
                      Dependencies & nodes,
                      bool required = false,
                      const GetNodeListOptions & options = GetNodeListOptions() ) const;

    // helpers to get properties
    bool GetString( const BFFToken * iter, const BFFVariable *& var, const char * name, bool required = false ) const;
    bool GetString( const BFFToken * iter, AString & var, const char * name, bool required = false ) const;
    bool GetStringOrArrayOfStrings( const BFFToken * iter, const BFFVariable *& var, const char * name, bool required ) const;
    bool GetStrings( const BFFToken * iter, Array<AString> & strings, const char * name, bool required = false ) const;

    // helper function to make alias for target
    bool ProcessAlias( NodeGraph & nodeGraph, Node * nodeToAlias ) const;
    bool ProcessAlias( NodeGraph & nodeGraph, Dependencies & nodesToAlias ) const;

    // Reflection based property population
    bool GetNameForNode( NodeGraph & nodeGraph, const BFFToken * iter, const ReflectionInfo * ri, AString & name ) const;
    bool PopulateProperties( NodeGraph & nodeGraph, const BFFToken * iter, Node * node ) const;
    bool PopulateProperties( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectionInfo * ri ) const;
    bool PopulateProperty( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateArrayOfStrings( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const;
    bool PopulateString( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable, bool required ) const;
    bool PopulateBool( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateInt32( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateUInt32( const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateArrayOfStructs( NodeGraph & nodeGraph, const BFFToken * iter, void * base, const ReflectedProperty & property, const BFFVariable * variable ) const;
    bool PopulateArrayOfStructsElement( NodeGraph & nodeGraph, const BFFToken * iter, void * structBase, const ReflectionInfo * structRI, const BFFVariable * srcVariable ) const;

    bool PopulateStringHelper( NodeGraph & nodeGraph, const BFFToken * iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, Array<AString> & outStrings ) const;
    bool PopulateStringHelper( NodeGraph & nodeGraph, const BFFToken * iter, const Meta_Path * pathMD, const Meta_File * fileMD, const Meta_AllowNonFile * allowNonFileMD, const BFFVariable * variable, const AString & string, Array<AString> & outStrings ) const;
    bool PopulatePathAndFileHelper( const BFFToken * iter, const Meta_Path * pathMD, const Meta_File * fileMD, const AString & variableName, AString & valueToFix ) const;
};

//------------------------------------------------------------------------------
