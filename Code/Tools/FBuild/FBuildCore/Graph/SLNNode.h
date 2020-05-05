// SLNNode.h - a node that builds a sln file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"
#include "Tools/FBuild/FBuildCore/Helpers/SLNGenerator.h"

#include "Core/Containers/Array.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class VSProjectBaseNode;

// SolutionConfigBase
//------------------------------------------------------------------------------
class SolutionConfigBase : public Struct
{
    REFLECT_STRUCT_DECLARE( SolutionConfigBase )
public:
    Array< AString > m_SolutionBuildProjects;
    Array< AString > m_SolutionDeployProjects;
};

// SolutionConfig
//------------------------------------------------------------------------------
class SolutionConfig : public SolutionConfigBase
{
    REFLECT_STRUCT_DECLARE( SolutionConfig )
public:
    SolutionConfig() = default;
    explicit SolutionConfig( const SolutionConfigBase & baseConfig )
        : SolutionConfigBase( baseConfig )
    {}

    AString m_SolutionPlatform;
    AString m_SolutionConfig;
    AString m_Platform;
    AString m_Config;

    bool operator < ( const SolutionConfig & other ) const
    {
        const int32_t cmpConfig = m_Config.CompareI( other.m_Config );
        return ( cmpConfig == 0 ) ? m_SolutionPlatform < other.m_SolutionPlatform
                                  : cmpConfig < 0;
    }
};

// SolutionFolder
//------------------------------------------------------------------------------
class SolutionFolder : public Struct
{
    REFLECT_STRUCT_DECLARE( SolutionFolder )
public:
    AString             m_Path;
    Array< AString >    m_Projects;
    Array< AString >    m_Items;
};

// SolutionDependency
//------------------------------------------------------------------------------
class SolutionDependency : public Struct
{
    REFLECT_STRUCT_DECLARE( SolutionDependency )
public:
    Array< AString >    m_Projects;
    Array< AString >    m_Dependencies;
};

// SLNNode
//------------------------------------------------------------------------------
class SLNNode : public FileNode
{
    REFLECT_NODE_DECLARE( SLNNode )
public:
    SLNNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~SLNNode() override;

    static inline Node::Type GetTypeS() { return Node::SLN_NODE; }

private:
    virtual BuildResult DoBuild( Job * job ) override;

    bool Save( const AString & content, const AString & fileName ) const;

    bool                    GatherProject( NodeGraph & nodeGraph,
                                           const Function * function,
                                           const BFFToken * iter,
                                           const char * propertyName,
                                           const AString & projectName,
                                           Array< VSProjectBaseNode * > & inOutProjects ) const;
    bool                    GatherProjects( NodeGraph & nodeGraph,
                                            const Function * function,
                                            const BFFToken * iter,
                                            const char * propertyName,
                                            const Array< AString > & projectNames,
                                            Array< VSProjectBaseNode * > & inOutProjects ) const;

    // Reflected
    Array< AString >            m_SolutionProjects;
    AString                     m_SolutionVisualStudioVersion;
    AString                     m_SolutionMinimumVisualStudioVersion;
    Array< SolutionConfig >     m_SolutionConfigs;
    Array< SolutionFolder >     m_SolutionFolders;
    Array< SolutionDependency > m_SolutionDependencies;
    SolutionConfigBase          m_BaseSolutionConfig;
};

//------------------------------------------------------------------------------
