// Function
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTION_H
#define FBUILD_FUNCTIONS_FUNCTION_H

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
class Meta_File;
class Meta_Path;
class Node;
class ReflectionInfo;

// Function
//------------------------------------------------------------------------------
class Function
{
public:
	explicit	Function( const char * functionName );
	virtual		~Function();

	// info about a function
	inline const AString & GetName() const { return m_Name; }

	// access to functions
	static const Function * Find( const AString & name );

	static void Create();
	static void Destroy();

	// does this function support a header? i.e. FunctionName( ...header.... )
	virtual bool AcceptsHeader() const; // can have a header
	virtual bool NeedsHeader() const;	// must have a header
	virtual bool NeedsBody() const;		// must have a body i.e. { ... }

	// must this function be unique?
	virtual bool IsUnique() const;
	inline bool GetSeen() const { return m_Seen; }
	inline void SetSeen() const { m_Seen = true; }

	// most functions don't need to override this
	virtual bool ParseFunction( const BFFIterator & functionNameStart,
								const BFFIterator * functionBodyStartToken, 
								const BFFIterator * functionBodyStopToken,
								const BFFIterator * functionHeaderStartToken,
								const BFFIterator * functionHeaderStopToken ) const;

	// most functions will override this to commit the effects of the function
	virtual bool Commit( const BFFIterator & funcStartIter ) const;

	// helpers to clean/fixup paths to files and folders
	static void CleanFolderPaths( Array< AString > & folders );
	static void CleanFilePaths( Array< AString > & files );
	void CleanFileNames( Array< AString > & fileNames ) const;

	bool GetDirectoryListNodeList( const BFFIterator & iter,
								   const Array< AString > & paths,
								   const Array< AString > & excludePaths,
                                   const Array< AString > & filesToExclude,
								   bool recurse,
								   const AString & pattern,
								   const char * inputVarName,
								   Dependencies & nodes ) const;
    bool GetFileNodes( const BFFIterator & iter,
                       const Array< AString > & files,
                       const char * inputVarName,
                       Dependencies & nodes ) const;
    bool GetObjectListNodes( const BFFIterator & iter,
                             const Array< AString > & objectLists,
                             const char * inputVarName,
                             Dependencies & nodes ) const;

	bool GetNodeList( const BFFIterator & iter, const char * name, Dependencies & nodes, bool required = false,
					  bool allowCopyDirNodes = false, bool allowUnityNodes = false ) const;

private:
	Function *	m_NextFunction;
	static Function * s_FirstFunction;

protected:
	AString		m_Name;
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
	bool GetFileNode( const BFFIterator & iter, Node * & fileNode, const char * name, bool required = false ) const;

	// helper function to make alias for target
	bool ProcessAlias( const BFFIterator & iter, Node * nodeToAlias ) const;
	bool ProcessAlias( const BFFIterator & iter, Dependencies & nodesToAlias ) const;

	// Reflection based property population
	bool GetNameForNode( const BFFIterator & iter, const ReflectionInfo * ri, AString & name ) const;
	bool PopulateProperties( const BFFIterator & iter, Node * node ) const;
	bool PopulateArrayOfStrings( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const;
	bool PopulateString( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const;
	bool PopulateBool( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const;
	bool PopulateUInt32( const BFFIterator & iter, Node * node, const ReflectedProperty & property, const BFFVariable * variable ) const;

	bool PopulatePathAndFileHelper( const BFFIterator & iter, const Meta_Path * pathMD, const Meta_File * fileMD, const AString & variableName, const AString & originalValue, AString & valueToFix ) const;
private:
	bool GetNodeListRecurse( const BFFIterator & iter, const char * name, Dependencies & nodes, const AString & nodeName,
							 bool allowCopyDirNodes, bool allowUnityNodes ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTION_H
