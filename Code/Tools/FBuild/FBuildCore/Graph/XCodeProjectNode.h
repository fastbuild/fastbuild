// XCodePorjectNode.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// XCodeProjectConfig
//------------------------------------------------------------------------------
struct XCodeProjectConfig : public Struct
{
    REFLECT_STRUCT_DECLARE( XCodeProjectConfig )
public:
    AString             m_Config;
    AString             m_Target;
    const Node *        m_TargetNode = nullptr;

    static bool ResolveTagets( NodeGraph & nodeGraph,
                               Array< XCodeProjectConfig > & configs,
                               const BFFIterator * iter = nullptr,
                               const Function * function = nullptr );
};

// XCodeProjectNode
//------------------------------------------------------------------------------
class XCodeProjectNode : public FileNode
{
    REFLECT_NODE_DECLARE( XCodeProjectNode )
public:
    explicit XCodeProjectNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~XCodeProjectNode();

    static inline Node::Type GetTypeS() { return Node::XCODEPROJECT_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    Array< AString >    m_ProjectInputPaths;
    Array< AString >    m_ProjectInputPathsExclude;
    Array< AString >    m_ProjectFiles;
    Array< AString >    m_ProjectFilesToExclude;
    Array< AString >    m_PatternToExclude;
    Array< AString >    m_ProjectBasePath;
    Array< AString >    m_ProjectAllowedFileExtensions;
    Array< XCodeProjectConfig > m_ProjectConfigs;
    AString             m_XCodeOrganizationName;
    AString             m_XCodeBuildToolPath;
    AString             m_XCodeBuildToolArgs;
    AString             m_XCodeBuildWorkingDir;
};

//------------------------------------------------------------------------------
