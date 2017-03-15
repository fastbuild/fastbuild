// CompilerNode.h - a node that manages a compiler executable
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// CompilerNode
//------------------------------------------------------------------------------
class CompilerNode : public FileNode
{
    REFLECT_NODE_DECLARE( CompilerNode )
public:
    explicit CompilerNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~CompilerNode();

    static inline Node::Type GetTypeS() { return Node::COMPILER_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

    inline const ToolManifest & GetManifest() const { return m_Manifest; }

    inline bool SimpleDistributionMode() const { return m_SimpleDistributionMode; }
    inline bool CanBeDistributed() const { return m_AllowDistribution; }
    #if defined( __WINDOWS__ )
        inline bool IsVS2012EnumBugFixEnabled() const { return m_VS2012EnumBugFix; }
    #endif
    inline bool IsClangRewriteIncludesEnabled() const { return m_ClangRewriteIncludes; }
private:
    virtual bool DetermineNeedToBuild( bool forceClean ) const override;
    virtual BuildResult DoBuild( Job * job ) override;

    // Exposed params
    Array< AString >    m_ExtraFiles;
    Array< AString >    m_CustomEnvironmentVariables;

    bool            m_AllowDistribution;
    bool            m_VS2012EnumBugFix;
    bool            m_ClangRewriteIncludes;
    AString         m_ExecutableRootPath;
    bool            m_SimpleDistributionMode;
    ToolManifest    m_Manifest;
};

//------------------------------------------------------------------------------
