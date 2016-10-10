// Mutex
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "Object.h"
#include "Core/Reflection/Container.h"

// ReflectionInfo
//------------------------------------------------------------------------------
// Struct_ReflectionInfo
//------------------------------------------------------------------------------
class Struct_ReflectionInfo : public ReflectionInfo
{
public:
    explicit Struct_ReflectionInfo() { SetTypeName( "Struct" ); m_IsAbstract = true; }
    virtual ~Struct_ReflectionInfo() = default;
};

// Object_ReflectionInfo
//------------------------------------------------------------------------------
class Object_ReflectionInfo : public ReflectionInfo
{
public:
    explicit Object_ReflectionInfo() { SetTypeName( "Object" ); m_IsAbstract = true; }
    virtual ~Object_ReflectionInfo() = default;
};
Struct_ReflectionInfo g_Struct_ReflectionInfo;
Object_ReflectionInfo g_Object_ReflectionInfo;

/*static*/ const ReflectionInfo * Object::GetReflectionInfoS()
{
    return &g_Object_ReflectionInfo;
}

void Object_ReflectionInfo_Bind()
{
    ReflectionInfo::BindReflection( g_Object_ReflectionInfo );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
Object::Object()
    : m_Id( 0 )
    , m_Parent( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Object::~Object() = default;

// GetScopedName
//------------------------------------------------------------------------------
void Object::GetScopedName( AString & scopedName ) const
{
    // recurse up the parents
    Array< const Object * > stack( 16, false );
    Object * obj = GetParent();
    while ( obj )
    {
        stack.Append( obj );
        obj = obj->GetParent();
    }

    // build name of parent objects
    size_t num = stack.GetSize();
    for ( size_t i=0; i<num; ++i )
    {
        scopedName += stack[ i ]->GetName();
        scopedName += '.';
    }

    // add our name
    scopedName += GetName();
}

//------------------------------------------------------------------------------
