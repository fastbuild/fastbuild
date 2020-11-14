// VCXProjectNode.h - a node that builds a vcxproj file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "VSProjectBaseNode.h"

#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

#include "Core/Containers/Array.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class DirectoryListNode;

// VSProjectConfigBase
//  - Options that can be set for all configs, or overridden per-config
//------------------------------------------------------------------------------
class VSProjectConfigBase : public Struct
{
    REFLECT_STRUCT_DECLARE( VSProjectConfigBase )
public:
    AString             m_ProjectBuildCommand;
    AString             m_ProjectRebuildCommand;
    AString             m_ProjectCleanCommand;
    AString             m_Output;
    AString             m_PreprocessorDefinitions;
    AString             m_IncludeSearchPath;
    AString             m_ForcedIncludes;
    AString             m_AssemblySearchPath;
    AString             m_ForcedUsingAssemblies;
    AString             m_AdditionalOptions;
    AString             m_OutputDirectory;
    AString             m_IntermediateDirectory;
    AString             m_BuildLogFile;
    AString             m_LayoutDir;
    AString             m_LayoutExtensionFilter;
    AString             m_Xbox360DebuggerCommand;
    AString             m_DebuggerFlavor;
    AString             m_AumidOverride;
    AString             m_PlatformToolset;
    AString             m_DeploymentType;
    AString             m_DeploymentFiles;
    AString             m_LocalDebuggerCommandArguments;
    AString             m_LocalDebuggerWorkingDirectory;
    AString             m_LocalDebuggerCommand;
    AString             m_LocalDebuggerEnvironment;
    AString             m_RemoteDebuggerCommand;
    AString             m_RemoteDebuggerCommandArguments;
    AString             m_RemoteDebuggerWorkingDirectory;
    AString             m_Keyword;
    AString             m_RootNamespace;
    AString             m_ApplicationType;
    AString             m_ApplicationTypeRevision;
    AString             m_TargetLinuxPlatform;
    AString             m_LinuxProjectType;
    AString             m_PackagePath;
    AString             m_AdditionalSymbolSearchPaths;
};

// VSProjectConfig
//  - Properties that are set per-config, overriding those set at the project
//    level, or those that can only be meaningfully set per-config
//------------------------------------------------------------------------------
class VSProjectConfig : public VSProjectConfigBase
{
    REFLECT_STRUCT_DECLARE( VSProjectConfig )
public:
    VSProjectConfig() = default;
    explicit VSProjectConfig( const VSProjectConfigBase & baseConfig )
        : VSProjectConfigBase( baseConfig )
    {}

    AString             m_Platform;
    AString             m_Config;
    AString             m_Target;
    const Node *        m_TargetNode = nullptr;

    static bool ResolveTargets( NodeGraph & nodeGraph,
                                Array< VSProjectConfig > & configs,
                                const BFFToken * iter = nullptr,
                                const Function * function = nullptr );
};

// VSProjectFileType
//------------------------------------------------------------------------------
class VSProjectFileType : public Struct
{
    REFLECT_STRUCT_DECLARE( VSProjectFileType )
public:
    AString             m_FileType; // e.g. "CppForm"
    AString             m_Pattern;  // e.g. "Code\Forms\*.h" (can be full filename also)
};

// VSProjectImport
//------------------------------------------------------------------------------
class VSProjectImport : public Struct
{
    REFLECT_STRUCT_DECLARE( VSProjectImport )
public:
    AString             m_Condition;    // e.g. "'$(ConfigurationType)' == 'Makefile'"
    AString             m_Project;      // e.g. "$(VCTargetsPath)\\Platforms\\$(Platform)\\SCE.Makefile.$(Platform).targets"
};

// VCXProjectNode
//------------------------------------------------------------------------------
class VCXProjectNode : public VSProjectBaseNode
{
    REFLECT_NODE_DECLARE( VCXProjectNode )
public:
    VCXProjectNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~VCXProjectNode() override;

    static inline Node::Type GetTypeS() { return Node::VCXPROJECT_NODE; }

private:
    virtual BuildResult DoBuild( Job * job ) override;
    virtual void PostLoad( NodeGraph & nodeGraph ) override;

    bool Save( const AString & content, const AString & fileName ) const;

    // VSProjectBaseNode interface
    virtual const AString & GetProjectTypeGuid() const override;

    // Exposed
    Array< AString >    m_ProjectInputPaths;
    Array< AString >    m_ProjectInputPathsExclude;
    Array< AString >    m_ProjectFiles;
    Array< AString >    m_ProjectFilesToExclude;
    Array< AString >    m_ProjectPatternToExclude;
    Array< AString >    m_ProjectBasePaths;
    Array< AString >    m_ProjectAllowedFileExtensions;
    VSProjectConfigBase m_BaseProjectConfig;
    Array< VSProjectConfig > m_ProjectConfigs;
    Array< VSProjectFileType > m_ProjectFileTypes;

    AString             m_DefaultLanguage;
    AString             m_ApplicationEnvironment;
    bool                m_ProjectSccEntrySAK = false;

    Array< AString >    m_ProjectReferences;
    Array< AString >    m_ProjectProjectReferences;

    Array< VSProjectImport > m_ProjectProjectImports;
};

//------------------------------------------------------------------------------
