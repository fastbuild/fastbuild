// BindReflection.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "BindReflection.h"

// BindReflection_Core
//------------------------------------------------------------------------------
void BindReflection_Core()
{
    BIND_REFLECTION( RefObject );
    BIND_REFLECTION( Object );
    BIND_REFLECTION( Container );
}

//------------------------------------------------------------------------------
