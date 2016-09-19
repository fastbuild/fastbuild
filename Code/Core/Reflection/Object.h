// Object.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ReflectionMacros.h"
#include "Core/Reflection/RefObject.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Container;

// Struct
//------------------------------------------------------------------------------
class Struct
{
public:
};

void Object_ReflectionInfo_Bind();

// Object
//------------------------------------------------------------------------------
class Object : public RefObject
{
public:
    explicit Object();
    virtual ~Object();
    virtual void Init() {}

    inline uint32_t GetId() const { return m_Id; }
    inline void SetName( const AString & name ) { m_Name = name; }
    inline const AString & GetName() const { return m_Name; }

    virtual const ReflectionInfo * GetReflectionInfoV() const = 0;

    static const ReflectionInfo * GetReflectionInfoS();

    void GetScopedName( AString & scopedName ) const;
    Container * GetParent() const { return m_Parent; }
protected:
    uint32_t    m_Id;
    AString     m_Name;
    Container * m_Parent;
};

//------------------------------------------------------------------------------
