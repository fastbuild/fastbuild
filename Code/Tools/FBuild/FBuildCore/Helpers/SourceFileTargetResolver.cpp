// SourceFileTargetResolver
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SourceFileTargetResolver.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Profile/Profile.h"

//------------------------------------------------------------------------------
SourceFileTargetResolver::SourceFileTargetResolver() = default;

//------------------------------------------------------------------------------
SourceFileTargetResolver::~SourceFileTargetResolver() = default;

//------------------------------------------------------------------------------
bool SourceFileTargetResolver::Resolve( const NodeGraph & nodeGraph,
                                        const Dependencies & hintTargets,
                                        const Array<AString> & sourceFiles,
                                        Array<AString> & outTargetNames )
{
    PROFILE_FUNCTION;

    // Resolver should only be used if we have source files to resolve
    ASSERT( sourceFiles.IsEmpty() == false );

    // Attempt to find targets that depend on this source file

    // Flag nodes as not visited so that we can track visits
    nodeGraph.SetBuildPassTagForAllNodes( eFindTargetsRecurseNotVisited );

    // For each sourceFile, find associated object files from the hint targets
    StackArray<const Node *> finalTargets;
    bool failedToFindATarget = false;
    for ( const AString & sourceFile : sourceFiles )
    {
        StackArray<const Node *> newTargets;

        // Canonicalize the name. FindNode will do this, but we want access
        // to the string and also want to only try the canonical path
        AStackString canonicalPath;
        NodeGraph::CleanPath( sourceFile, canonicalPath );

        // Get node for source file
        const Node * node = nodeGraph.FindNode( canonicalPath );
        if ( node )
        {
            for ( const Dependency & targetHint : hintTargets )
            {
                FindTargetsRecurse( node, targetHint.GetNode(), newTargets );
            }
        }

        if ( newTargets.IsEmpty() )
        {
            FLOG_OUTPUT( "Unable to determine target for '%s'\n", canonicalPath.Get() );
            failedToFindATarget = true;
        }
        else
        {
            finalTargets.Append( newTargets );
        }
    }

    // If any targets failed to resolve, use the hint list instead
    // This allows "first time" builds to still do something useful
    if ( failedToFindATarget )
    {
        FLOG_OUTPUT( "Falling back to build hint target(s)\n" );
        finalTargets.Clear();
        for ( const Dependency & dep : hintTargets )
        {
            finalTargets.EmplaceBack( dep.GetNode() );
        }
    }

    // Reset tag state
    nodeGraph.SetBuildPassTagForAllNodes( 0 );

    //
    outTargetNames.SetCapacity( finalTargets.GetSize() );
    for ( const Node * target : finalTargets )
    {
        outTargetNames.EmplaceBack( target->GetName() );
    }
    return true;
}

//------------------------------------------------------------------------------
void SourceFileTargetResolver::FindTargetsRecurse( const Node * sourceFile,
                                                   const Node * possibleTarget,
                                                   Array<const Node *> & inoutTargets ) const
{
    // Avoid redundant recursions
    if ( possibleTarget->GetBuildPassTag() == eFindTargetsRecurseVisited )
    {
        return;
    }
    possibleTarget->SetBuildPassTag( eFindTargetsRecurseVisited );

    // Check for direct dependencies
    const Dependencies * depsToCheck[] = { &possibleTarget->GetStaticDependencies(),
                                           &possibleTarget->GetDynamicDependencies() };
    for ( const Dependencies * deps : depsToCheck )
    {
        for ( const Dependency & dep : *deps )
        {
            // Does this node depend on the source file?
            if ( dep.GetNode() == sourceFile )
            {
                // This node directly depends on the source file, so the target
                // should be included
                inoutTargets.EmplaceBack( possibleTarget );

                // No need to check remaining dependencies at this level as
                // they would only be duplicative and no need to check
                // recursively at the target at this level is guaranteed to
                // be a superset of anything it depends upon.
                return;
            }
        }
    }

    // Check for indirect dependencies
    for ( const Dependencies * deps : depsToCheck )
    {
        for ( const Dependency & dep : *deps )
        {
            // Check for indirect dependencies. There can be several, so even
            // if we find something, we need to check them all
            FindTargetsRecurse( sourceFile, dep.GetNode(), inoutTargets );
        }
    }
}

//------------------------------------------------------------------------------
