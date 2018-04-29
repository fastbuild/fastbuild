// FunctionSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionSettings.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"

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
/*virtual*/ bool FunctionSettings::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
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
    // "RootPath"
    AStackString<> rootPath;
    if ( !GetString( funcStartIter, rootPath, ".RootPath" ) )
    {
        return false;
    }
    FBuild::Get().SetRootPath( rootPath );
    if ( !rootPath.IsEmpty() )
    {
        FLOG_INFO( "RootPath: '%s'", rootPath.Get() );
    }


    return settingsNode->Initialize( nodeGraph, funcStartIter, this );
}

//------------------------------------------------------------------------------
