// BFFVariableScope.h - the collection of variables within a single scope or struct
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

// Core
#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"

// BFFVariableScope
//------------------------------------------------------------------------------
using BFFVariableScope = UnorderedMap<AString, BFFVariable>;
