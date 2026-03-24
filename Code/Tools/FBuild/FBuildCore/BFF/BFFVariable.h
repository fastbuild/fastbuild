// BFFVariable - a single variable in a BFFStack
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFToken;

// BFFVariable
//------------------------------------------------------------------------------
class BFFVariable
{
public:
    const AString & GetName() const { return m_Name; }

    const AString & GetString() const
    {
        ASSERT( IsString() );
        return m_StringValue;
    }
    const Array<AString> & GetArrayOfStrings() const
    {
        ASSERT( IsArrayOfStrings() );
        return m_ArrayValues;
    }
    int32_t GetInt() const
    {
        ASSERT( IsInt() );
        return m_IntValue;
    }
    bool GetBool() const
    {
        ASSERT( IsBool() );
        return m_BoolValue;
    }
    const Array<BFFVariable> & GetStructMembers() const
    {
        ASSERT( IsStruct() );
        return m_SubVariables;
    }
    const Array<BFFVariable> & GetArrayOfStructs() const
    {
        ASSERT( IsArrayOfStructs() );
        return m_SubVariables;
    }

    enum VarType : uint8_t
    {
        VAR_ANY = 0, // used for searching
        VAR_STRING = 1,
        VAR_BOOL = 2,
        VAR_ARRAY_OF_STRINGS = 3,
        VAR_INT = 4,
        VAR_STRUCT = 5,
        VAR_ARRAY_OF_STRUCTS = 6,
        MAX_VAR_TYPES    // NOTE: Be sure to update s_TypeNames when adding to here
    };

    VarType GetType() const { return m_Type; }
    static const char * GetTypeName( VarType t ) { return s_TypeNames[ (uint32_t)t ]; }

    bool IsString() const { return m_Type == BFFVariable::VAR_STRING; }
    bool IsBool() const { return m_Type == BFFVariable::VAR_BOOL; }
    bool IsArrayOfStrings() const { return m_Type == BFFVariable::VAR_ARRAY_OF_STRINGS; }
    bool IsInt() const { return m_Type == BFFVariable::VAR_INT; }
    bool IsStruct() const { return m_Type == BFFVariable::VAR_STRUCT; }
    bool IsArrayOfStructs() const { return m_Type == BFFVariable::VAR_ARRAY_OF_STRUCTS; }

    bool Frozen() const { return m_FreezeCount > 0; }
    void Freeze() const { ++m_FreezeCount; }
    void Unfreeze() const
    {
        ASSERT( m_FreezeCount != 0 );
        --m_FreezeCount;
    }

    static const BFFVariable * GetMemberByName( const AString & name, const Array<BFFVariable> & members );
private:
    static BFFVariable * GetMemberByName( const AString & name, Array<BFFVariable> & members );

public:
    const BFFToken & GetToken() const { return m_Token; }

    explicit BFFVariable( const BFFVariable & other );
    explicit BFFVariable( BFFVariable && other );
    ~BFFVariable() = default;

private:
    friend class BFFStackFrame;

    explicit BFFVariable( const AString & name, const BFFToken & token, VarType type );
    explicit BFFVariable( const AString & name, const BFFToken & token, const AString & value );
    explicit BFFVariable( const AString & name, const BFFToken & token, bool value );
    explicit BFFVariable( const AString & name, const BFFToken & token, const Array<AString> & values );
    explicit BFFVariable( const AString & name, const BFFToken & token, int32_t i );
    explicit BFFVariable( const AString & name, const BFFToken & token, const Array<BFFVariable> & values );
    explicit BFFVariable( const AString & name, const BFFToken & token, Array<BFFVariable> && values );
    explicit BFFVariable( const AString & name, const BFFToken & token, const Array<BFFVariable> & structs, VarType type ); // type for disambiguation

    BFFVariable & operator=( const BFFVariable & other ) = delete;

    void ForceSetValueString( const AString & value );
    void ForceSetValueBool( bool value );
    void ForceSetValueArrayOfStrings( const Array<AString> & values );
    void ForceSetValueArrayOfStrings( Array<AString> && values );
    void ForceSetValueArrayOfStrings( const AString & value );
    void ForceSetValueInt( int i );
    void ForceSetValueStruct( const Array<BFFVariable> & members );
    void ForceSetValueStruct( Array<BFFVariable> && members );
    void ForceSetValueArrayOfStructs( const Array<BFFVariable> & values );
    void ForceSetValueArrayOfStructs( Array<BFFVariable> && values );
    void ForceSetValueArrayOfStructs( const BFFVariable & value );

public:
    bool Set( const BFFVariable & src, const BFFToken * operatorIter );

    template <BFFVariable::VarType SrcType, class V>
    bool SetValue( V value, const BFFToken * operatorIter );

    bool Concat( const BFFVariable & src, const BFFToken * operatorIter );
    bool Concat( BFFVariable && src, const BFFToken * operatorIter );

    template <BFFVariable::VarType SrcType, class V>
    bool ConcatValue( V value, const BFFToken * operatorIter );

    bool Subtract( const BFFVariable & src, const BFFToken * operatorIter );

    template <BFFVariable::VarType SrcType, class V>
    bool SubtractValue( const V & value, const BFFToken * operatorIter );

private:
    void SetType( VarType type );

    AString m_Name;
    VarType m_Type;

    mutable uint8_t m_FreezeCount = 0;

    //
    bool m_BoolValue = false;
    int32_t m_IntValue = 0;
    AString m_StringValue;
    Array<AString> m_ArrayValues;
    Array<BFFVariable> m_SubVariables; // Used for struct members of arrays of structs
    const BFFToken & m_Token;

    static const char * s_TypeNames[ MAX_VAR_TYPES ];
};

//------------------------------------------------------------------------------
