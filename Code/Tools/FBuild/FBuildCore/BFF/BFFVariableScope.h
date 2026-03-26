// BFFVariableScope.h - the collection of variables within a single scope or struct
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class BFFVariable;

// Core
#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"

// BFFVariableScope
//------------------------------------------------------------------------------
using BFFVariableScope = UnorderedMap<AString, BFFVariable>;
