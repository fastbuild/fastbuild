// FunctionSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionSettings.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"

#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionSettings::FunctionSettings()
: Function( "Settings" )
{
}

// IsUnique
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSettings::IsUnique() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSettings::Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const
{
    AStackString<> name( "$$Settings$$" );
    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    SettingsNode * settingsNode = nodeGraph.CreateSettingsNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, settingsNode ) )
    {
        return false;
    }

    return settingsNode->Initialize( nodeGraph, funcStartIter, this );
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionSettings::CreateNode() const
{
    return FNEW( SettingsNode );
}

//------------------------------------------------------------------------------
