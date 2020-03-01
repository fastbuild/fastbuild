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
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~TextFileNode() override;

    static inline Node::Type GetTypeS() { return Node::TEXT_FILE_NODE; }

private:
    virtual bool DetermineNeedToBuild( const Dependencies & deps ) const override;
    virtual BuildResult DoBuild( Job * job ) override;

    void EmitCompilationMessage() const;

    // Exposed Properties
    AString             m_TextFileOutput;
    Array< AString >    m_TextFileInputStrings;
    bool                m_TextFileAlways;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
