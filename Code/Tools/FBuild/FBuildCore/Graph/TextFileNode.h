// TextFileNode
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------

// TextFileNode
//------------------------------------------------------------------------------
class TextFileNode : public FileNode
{
    REFLECT_NODE_DECLARE( TextFileNode )
public:
    explicit TextFileNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~TextFileNode() override;

    static inline Node::Type GetTypeS() { return Node::TEXT_FILE_NODE; }

private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual bool DetermineNeedToBuild( bool forceClean ) const override;
    virtual BuildResult DoBuild( Job * job ) override;

    // const FileNode * GetTextFileutable() const { return m_StaticDependencies[0].GetNode()->CastTo< FileNode >(); }
    // void GetFullArgs(AString & fullArgs) const;
    // void GetInputFiles(AString & fullArgs, const AString & pre, const AString & post) const;

    void EmitCompilationMessage() const;

    // Exposed Properties
    AString             m_TextFileOutput;
    Array< AString >    m_TextFileInputStrings;
    bool                m_TextFileAlways;
    Array< AString >    m_PreBuildDependencyNames;

    // Internal State
    AString             m_TextFileContents;
    // uint32_t            m_NumTextFileInputFiles;
};

//------------------------------------------------------------------------------
