// TextReader.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ConstMemoryStream;
class Object;
class RefObject;
class ReflectedProperty;
class ReflectionInfo;
class Struct;

// TextWriter
//------------------------------------------------------------------------------
class TextReader
{
public:
    explicit TextReader( ConstMemoryStream & stream );

    RefObject * Read();
private:
    bool ReadLines();
    bool ReadObject();
    bool ReadStruct();
    bool ReadRef();
    bool ReadWeakRef();
    bool ReadArray();
    bool ReadArrayOfStruct();
    bool ReadChildren();
    bool ReadProperty( PropertyType propertyType );

    void SkipWhitespace( bool stopAtLineEnd = false );
    bool GetToken( AString & token );
    bool GetString( AString & string );

    void Error( const char * error ) const;

    void ResolveWeakRefs() const;

    const char * m_Pos;
    const char * m_End;

    struct StackFrame
    {
        void * m_Base; // The pointer to the object/struct/etc being deserialized
        const ReflectionInfo * m_Reflection; // Reflection representing the data
        const ReflectedProperty * m_ArrayProperty; // are we processing an array?

        // Debug helpers
        #ifdef DEBUG
            RefObject * m_RefObject;
            Struct *    m_Struct;
        #endif
    };

    struct UnresolvedWeakRef
    {
        void * m_Base;
        const ReflectionInfo * m_Reflection;
        AString m_WeakRefName;
        AString m_WeakRefValue;
    };

    RefObject * m_RootObject;
    Array< StackFrame > m_DeserializationStack;
    Array< Object * > m_ObjectsToInit;
    Array< UnresolvedWeakRef > m_UnresolvedWeakRefs;
};

//------------------------------------------------------------------------------
