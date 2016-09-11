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

// LinkerNode
//------------------------------------------------------------------------------
class LinkerNode : public FileNode
{
public:
    explicit LinkerNode( const AString & linkerOutputName,
                         const Dependencies & inputLibraries,
                         const Dependencies & otherLibraries,
                         const AString & linkerType,
                         const AString & linker,
                         const AString & linkerArgs,
                         uint32_t flags,
                         const Dependencies & assemblyResources,
                         const AString & importLibName,
                         Node * linkerStampExe,
                         const AString & linkerStampExeArgs );
    virtual ~LinkerNode();

    enum Flag
    {
        LINK_FLAG_MSVC      = 0x01,
        LINK_FLAG_DLL       = 0x02,
        LINK_OBJECTS        = 0x04,
        LINK_FLAG_GCC       = 0x08,
        LINK_FLAG_SNC       = 0x10,
        LINK_FLAG_ORBIS_LD  = 0x20,
        LINK_FLAG_INCREMENTAL = 0x40,
        LINK_FLAG_GREENHILLS_ELXR = 0x80,
        LINK_FLAG_CODEWARRIOR_LD=0x100
    };

    inline bool IsADLL() const { return GetFlag( LINK_FLAG_DLL ); }

    virtual void Save( IOStream & stream ) const;

    static uint32_t DetermineLinkerTypeFlags( const AString & linkerType, const AString & linkerName );
    static uint32_t DetermineFlags( const AString & linkerType, const AString & linkerName, const AString & args );

    static bool IsLinkerArg_MSVC( const AString & token, const char * arg );
    static bool IsStartOfLinkerArg_MSVC( const AString & token, const char * arg );

protected:
    virtual BuildResult DoBuild( Job * job );

    void DoPreLinkCleanup() const;

    bool BuildArgs( Args & fullArgs ) const;
    void GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const;
    void GetInputFiles( Node * n, Args & fullArgs, const AString & pre, const AString & post ) const;
    void GetAssemblyResourceFiles( Args & fullArgs, const AString & pre, const AString & post ) const;
    void EmitCompilationMessage( const Args & fullArgs ) const;
    void EmitStampMessage() const;

    inline bool GetFlag( Flag flag ) const { return ( ( m_Flags & (uint32_t)flag ) != 0 ); }

    inline const char * GetDLLOrExe() const { return GetFlag( LINK_FLAG_DLL ) ? "DLL" : "Exe"; }

    bool CanUseResponseFile() const;

    AString m_LinkerType;
    AString m_Linker;
    AString m_LinkerArgs;
    uint32_t m_Flags;
    Dependencies m_AssemblyResources;
    Dependencies m_OtherLibraries;
    AString m_ImportLibName;
    const Node * m_LinkerStampExe;
    AString m_LinkerStampExeArgs;
};

//------------------------------------------------------------------------------
