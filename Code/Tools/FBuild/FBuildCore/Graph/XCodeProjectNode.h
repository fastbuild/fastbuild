// XCodePorjectNode.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// XCodeProjectConfig
//------------------------------------------------------------------------------
class XCodeProjectConfig : public ProjectGeneratorBaseConfig
{
    REFLECT_STRUCT_DECLARE( XCodeProjectConfig )
public:
    AString             m_Target;
    AString             m_XCodeBaseSDK;
    AString             m_XCodeDebugWorkingDir;

    static bool ResolveTargets( NodeGraph & nodeGraph,
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
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~XCodeProjectNode() override;

    static inline Node::Type GetTypeS() { return Node::XCODEPROJECT_NODE; }

private:
    virtual bool DetermineNeedToBuild( bool forceClean ) const override;
    virtual BuildResult DoBuild( Job * job ) override;
    virtual void PostLoad( NodeGraph & nodeGraph ) override;

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
    bool                m_XCodeDocumentVersioning = false;
    Array<AString>      m_XCodeCommandLineArguments;
    Array<AString>      m_XCodeCommandLineArgumentsDisabled;
};

//------------------------------------------------------------------------------
