// LinkerNode.h - builds an exe or dll
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
enum class ArgsResponseFileMode : uint32_t;

// LinkerNode
//------------------------------------------------------------------------------
class LinkerNode : public FileNode
{
    REFLECT_NODE_DECLARE( LinkerNode )
public:
    explicit LinkerNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~LinkerNode() override;

    enum Flag
    {
        LINK_FLAG_MSVC      = 0x01,
        LINK_FLAG_DLL       = 0x02,
        LINK_FLAG_GCC       = 0x08,
        LINK_FLAG_SNC       = 0x10,
        LINK_FLAG_ORBIS_LD  = 0x20,
        LINK_FLAG_INCREMENTAL = 0x40,
        LINK_FLAG_GREENHILLS_ELXR = 0x80,
        LINK_FLAG_CODEWARRIOR_LD=0x100,
        LINK_FLAG_WARNINGS_AS_ERRORS_MSVC = 0x200,
    };

    inline bool IsADLL() const { return GetFlag( LINK_FLAG_DLL ); }

    static uint32_t DetermineLinkerTypeFlags( const AString & linkerType, const AString & linkerName );
    static uint32_t DetermineFlags( const AString & linkerType, const AString & linkerName, const AString & args );

    static bool IsLinkerArg_MSVC( const AString & token, const char * arg );
    static bool IsStartOfLinkerArg_MSVC( const AString & token, const char * arg );

    static bool IsStartOfLinkerArg( const AString & token, const char * arg );

protected:
    friend class TestLinker;

    virtual BuildResult DoBuild( Job * job ) override;

    bool DoPreLinkCleanup() const;

    bool BuildArgs( Args & fullArgs ) const;
    void GetInputFiles( const AString & token, Args & fullArgs ) const;
    void GetInputFiles( Args & fullArgs, uint32_t startIndex, uint32_t endIndex, const AString & pre, const AString & post ) const;
    void GetInputFiles( Node * n, Args & fullArgs, const AString & pre, const AString & post ) const;
    void GetAssemblyResourceFiles( Args & fullArgs, const AString & pre, const AString & post ) const;
    void EmitCompilationMessage( const Args & fullArgs ) const;
    void EmitStampMessage() const;

    inline bool GetFlag( Flag flag ) const { return ( ( m_Flags & (uint32_t)flag ) != 0 ); }

    inline const char * GetDLLOrExe() const { return GetFlag( LINK_FLAG_DLL ) ? "DLL" : "Exe"; }

    ArgsResponseFileMode GetResponseFileMode() const;

    void GetImportLibName( const AString & args, AString & importLibName ) const;

    static bool GetOtherLibraries( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function, const AString & args, Dependencies & otherLibraries, bool msvc );
    static bool GetOtherLibrary( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function, Dependencies & libs, const AString & path, const AString & lib, bool & found );
    static bool GetOtherLibrary( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function, Dependencies & libs, const Array< AString > & paths, const AString & lib );
    static bool GetOtherLibsArg( const char * arg,
                                 AString & value,
                                 const AString * & it,
                                 const AString * const & end,
                                 bool canonicalizePath,
                                 bool isMSVC );
    static bool GetOtherLibsArg( const char * arg,
                                 Array< AString > & list,
                                 const AString * & it,
                                 const AString * const & end,
                                 bool canonicalizePath,
                                 bool isMSVC );

    static bool DependOnNode( NodeGraph & nodeGraph,
                              const BFFToken * iter,
                              const Function * function,
                              const AString & nodeName,
                              Dependencies & nodes );
    static bool DependOnNode( const BFFToken * iter,
                              const Function * function,
                              Node * node,
                              Dependencies & nodes );

    // Reflected
    AString             m_Linker;
    AString             m_LinkerOptions;
    AString             m_LinkerType;
    Array< AString >    m_Libraries;
    Array< AString >    m_Libraries2;
    Array< AString >    m_LinkerAssemblyResources;
    bool                m_LinkerLinkObjects             = false;
    bool                m_LinkerAllowResponseFile;
    bool                m_LinkerForceResponseFile;    
    AString             m_LinkerStampExe;
    AString             m_LinkerStampExeArgs;
    Array< AString >    m_PreBuildDependencyNames;
    Array< AString >    m_Environment;

    // Internal State
    uint32_t            m_Libraries2StartIndex          = 0;
    uint32_t            m_Flags                         = 0;
    uint32_t            m_AssemblyResourcesStartIndex   = 0;
    uint32_t            m_AssemblyResourcesNum          = 0;
    AString             m_ImportLibName;
    mutable const char * m_EnvironmentString            = nullptr;
};

//------------------------------------------------------------------------------
