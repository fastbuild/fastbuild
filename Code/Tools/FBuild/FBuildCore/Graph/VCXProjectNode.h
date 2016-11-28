// VCXProjectNode.h - a node that builds a vcxproj file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

#include "Core/Containers/Array.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class DirectoryListNode;

// UnityNode
//------------------------------------------------------------------------------
class VCXProjectNode : public FileNode
{
public:
    explicit VCXProjectNode( const AString & projectOutput,
                             const Array< AString > & projectBasePaths,
                             const Dependencies & paths,
                             const Array< AString > & pathsToExclude,
                             const Array< AString > & patternToExclude,
                             const Array< AString > & files,
                             const Array< AString > & filesToExclude,
                             const AString & rootNamespace,
                             const AString & projectGuid,
                             const AString & defaultLanguage,
                             const AString & applicationEnvironment,
                             const Array< VSProjectConfig > & configs,
                             const Array< VSProjectFileType > & fileTypes,
                             const Array< AString > & references,
                             const Array< AString > & projectReferences );
    virtual ~VCXProjectNode();

    static inline Node::Type GetTypeS() { return Node::VCXPROJECT_NODE; }

    const AString & GetProjectGuid() const { return m_ProjectGuid; }
    const Array< VSProjectConfig > & GetConfigs() const { return m_Configs; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    bool Save( const AString & content, const AString & fileName ) const;

    // TODO: Move into base class (share with UnityNode)
    void GetFiles( Array< FileIO::FileInfo * > & files ) const;

    void AddFile( VSProjectGenerator & pg, const AString & fileName ) const;

    Array< AString >    m_ProjectBasePaths;
    Array< AString >    m_PathsToExclude;
    Array< AString >    m_PatternToExclude;
    Array< AString >    m_Files;
    Array< AString >    m_FilesToExclude;
    AString             m_RootNamespace;
    AString             m_ProjectGuid;
    AString             m_DefaultLanguage;
    AString             m_ApplicationEnvironment;
    Array< VSProjectConfig > m_Configs;
    Array< VSProjectFileType > m_FileTypes;
    Array< AString >    m_References;
    Array< AString >    m_ProjectReferences;
};

//------------------------------------------------------------------------------
