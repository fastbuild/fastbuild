// Struct
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Struct.h"

// Core
#include "Core/Reflection/ReflectionInfo.h"

// Struct_ReflectionInfo
//------------------------------------------------------------------------------
class Struct_ReflectionInfo : public ReflectionInfo
{
public:
    explicit Struct_ReflectionInfo() { SetTypeName( "Struct" ); m_IsAbstract = true; }
    virtual ~Struct_ReflectionInfo() override = default;
};
Struct_ReflectionInfo g_Struct_ReflectionInfo;

//------------------------------------------------------------------------------
