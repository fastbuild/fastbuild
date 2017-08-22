// LibraryNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ObjectListNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class BFFIterator;
class CompilerNode;
class Function;
class NodeGraph;
class ObjectNode;

// LibraryNode
//------------------------------------------------------------------------------
class LibraryNode : public ObjectListNode
{
    REFLECT_NODE_DECLARE( LibraryNode )
public:
    LibraryNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~LibraryNode();

    static inline Node::Type GetTypeS() { return Node::LIBRARY_NODE; }

    virtual bool IsAFile() const override;

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

    enum Flag
    {
        LIB_FLAG_LIB    = 0x01, // MSVC style lib.exe
        LIB_FLAG_AR     = 0x02, // gcc/clang style ar.exe
        LIB_FLAG_ORBIS_AR=0x04, // Orbis ar.exe
        LIB_FLAG_GREENHILLS_AX=0x08, // Greenhills (WiiU) ax.exe
        LIB_FLAG_WARNINGS_AS_ERRORS_MSVC = 0x10,
    };
    static uint32_t DetermineFlags( const AString & librarianName, const AString & args );
private:
    friend class FunctionLibrary;

    virtual bool GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    // internal helpers
    bool BuildArgs( Args & fullArgs ) const;
    void EmitCompilationMessage( const Args & fullArgs ) const;
    FileNode * GetLibrarian() const;

    inline bool GetFlag( Flag flag ) const { return ( ( m_LibrarianFlags & (uint32_t)flag ) != 0 ); }

    bool CanUseResponseFile() const;

    // Exposed Properties
    AString             m_Librarian;
    AString             m_LibrarianOptions;
    AString             m_LibrarianOutput;
    Array< AString >    m_LibrarianAdditionalInputs;

    // Internal State
    uint32_t            m_NumLibrarianAdditionalInputs  = 0;
    uint32_t            m_LibrarianFlags                = 0;
};

//------------------------------------------------------------------------------
