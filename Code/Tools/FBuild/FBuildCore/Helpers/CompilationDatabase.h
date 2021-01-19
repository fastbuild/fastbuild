// CompilationDatabase - Generate a JSON Compilation Database
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Dependencies;
class NodeGraph;
class ObjectListNode;

// CompilationDatabase
//------------------------------------------------------------------------------
class CompilationDatabase
{
public:
    CompilationDatabase();
    ~CompilationDatabase();

    const AString & Generate( const NodeGraph & nodeGraph, Dependencies & dependencies );

protected:
    struct ObjectListContext
    {
        CompilationDatabase * m_DB;
        ObjectListNode * m_ObjectListNode;
        AString m_CompilerEscaped;
        Array< AString > m_ArgumentsEscaped;
    };

    void VisitNodes( const NodeGraph & nodeGraph, const Dependencies & dependencies, Array< bool > & visited );
    void HandleObjectListNode( const NodeGraph & nodeGraph, ObjectListNode * node );
    static void HandleInputFile( const AString & inputFile, const AString & baseDir, void * userData );
    void HandleInputFile( const AString & inputFile, const AString & baseDir, ObjectListContext * ctx );

    static void JSONEscape( AString & string );
    static void Unquote( AString & string );

    AString m_Output;
    AString m_DirectoryEscaped;
};

//------------------------------------------------------------------------------
