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
class VCXProjectNode;

// SolutionConfig
//------------------------------------------------------------------------------
class SolutionConfig : public Struct
{
    REFLECT_STRUCT_DECLARE( SolutionConfig )
public:
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
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~SLNNode();

    static inline Node::Type GetTypeS() { return Node::SLN_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    bool Save( const AString & content, const AString & fileName ) const;

    bool                    GatherProject( NodeGraph & nodeGraph, 
                                           const Function * function, 
                                           const BFFIterator & iter,
                                           const char * propertyName,
                                           const AString & projectName,
                                           Array< VCXProjectNode * > & inOutProjects ) const;
    bool                    GatherProjects( NodeGraph & nodeGraph, 
                                            const Function * function, 
                                            const BFFIterator & iter,
                                            const char * propertyName,
                                            const Array< AString > & projectNames,
                                            Array< VCXProjectNode * > & inOutProjects ) const;

    // Reflected
    Array< AString >            m_SolutionProjects;
    Array< AString >            m_SolutionBuildProjects;
    AString                     m_SolutionVisualStudioVersion;
    AString                     m_SolutionMinimumVisualStudioVersion;
    Array< SolutionConfig >     m_SolutionConfigs;
    Array< SolutionFolder >     m_SolutionFolders;
    Array< SolutionDependency > m_SolutionDependencies;
};

//------------------------------------------------------------------------------
