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
class CompilerNode;
class Function;
class NodeGraph;
class ObjectNode;
enum class ArgsResponseFileMode : uint32_t;

// LibraryNode
//------------------------------------------------------------------------------
class LibraryNode : public ObjectListNode
{
    REFLECT_NODE_DECLARE( LibraryNode )
public:
    LibraryNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~LibraryNode() override;

    static inline Node::Type GetTypeS() { return Node::LIBRARY_NODE; }

    virtual bool IsAFile() const override;

    enum Flag
    {
        LIB_FLAG_LIB    = 0x01, // MSVC style lib.exe
        LIB_FLAG_AR     = 0x02, // gcc/clang style ar.exe
        LIB_FLAG_ORBIS_AR=0x04, // Orbis ar.exe
        LIB_FLAG_GREENHILLS_AX=0x08, // Greenhills (WiiU) ax.exe
        LIB_FLAG_WARNINGS_AS_ERRORS_MSVC = 0x10,
    };
    static uint32_t DetermineFlags( const AString & librarianType, const AString & librarianName, const AString & args );
private:
    friend class FunctionLibrary;

    virtual bool GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    // internal helpers
    bool BuildArgs( Args & fullArgs ) const;
    void EmitCompilationMessage( const Args & fullArgs ) const;

    inline bool GetFlag( Flag flag ) const { return ( ( m_LibrarianFlags & (uint32_t)flag ) != 0 ); }

    ArgsResponseFileMode GetResponseFileMode() const;

    // Exposed Properties
    AString             m_Librarian;
    AString             m_LibrarianOptions;
    AString             m_LibrarianType;
    AString             m_LibrarianOutput;
    Array< AString >    m_LibrarianAdditionalInputs;
    Array< AString >    m_Environment;
    bool                m_LibrarianAllowResponseFile;
    bool                m_LibrarianForceResponseFile;

    // Internal State
    uint32_t            m_NumLibrarianAdditionalInputs  = 0;
    uint32_t            m_LibrarianFlags                = 0;
    mutable const char * m_EnvironmentString            = nullptr;
};

//------------------------------------------------------------------------------
