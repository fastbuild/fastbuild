// SLNNode.h - a node that builds a sln file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_SLNNODE_H
#define FBUILD_GRAPH_SLNNODE_H

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

// SLNNode
//------------------------------------------------------------------------------
class SLNNode : public FileNode
{
public:
    explicit SLNNode(   const AString & solutionOutput,
                        const AString & solutionBuildProject,
                        const AString & solutionVisualStudioVersion,
                        const AString & solutionMinimumVisualStudioVersion,
                        const Array< VSProjectConfig > & configs,
                        const Array< VCXProjectNode * > & projects,
                        const Array< SLNSolutionFolder > & folders );
    virtual ~SLNNode();

    static inline Node::Type GetTypeS() { return Node::SLN_NODE; }

    static Node * Load( IOStream & stream );
    virtual void Save( IOStream & stream ) const;
private:
    virtual BuildResult DoBuild( Job * job );

    bool Save( const AString & content, const AString & fileName ) const;

    AString m_SolutionBuildProject;
    AString m_SolutionVisualStudioVersion;
    AString m_SolutionMinimumVisualStudioVersion;

    Array< VSProjectConfig > m_Configs;
    Array< SLNSolutionFolder > m_Folders;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_SLNNODE_H
