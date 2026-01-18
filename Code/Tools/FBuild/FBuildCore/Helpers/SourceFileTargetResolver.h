// SourceFileTargetResolver
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class Dependencies;
class Node;
class NodeGraph;
template <class T> class Array;

//------------------------------------------------------------------------------
class SourceFileTargetResolver
{
public:
    SourceFileTargetResolver();
    ~SourceFileTargetResolver();

    [[nodiscard]] bool Resolve( const NodeGraph & nodeGraph,
                                const Dependencies & hintTargets,
                                const Array<AString> & sourceFiles,
                                Array<AString> & outTargetNames );

protected:
    void FindTargetsRecurse( const Node * sourceFile,
                             const Node * possibleTarget,
                             Array<const Node *> & inoutTargets ) const;

    enum : uint32_t
    {
        eFindTargetsRecurseNotVisited = 0,
        eFindTargetsRecurseVisited = 1,
    };
};

//------------------------------------------------------------------------------
