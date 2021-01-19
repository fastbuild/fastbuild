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

// Helpers
//------------------------------------------------------------------------------
// Used to avoid aliasing when const casting (can't use const_cast due to the
// desired const being part of the template type)
#define RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( input )       \
    union                                                   \
    {                                                       \
        const Array< BFFVariable * > *          normal;     \
        const Array< const BFFVariable * > *    constified; \
    };                                                      \
    normal = &input;                                        \
    return *constified;

// BFFVariable
//------------------------------------------------------------------------------
class BFFVariable
{
public:
    inline const AString & GetName() const { return m_Name; }

    const AString & GetString() const { ASSERT( IsString() ); return m_StringValue; }
    const Array< AString > & GetArrayOfStrings() const { ASSERT( IsArrayOfStrings() ); return m_ArrayValues; }
    int32_t GetInt() const { ASSERT( IsInt() ); return m_IntValue; }
    bool GetBool() const { ASSERT( IsBool() ); return m_BoolValue; }
    const Array< const BFFVariable * > & GetStructMembers() const { ASSERT( IsStruct() ); RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( m_SubVariables ) }
    const Array< const BFFVariable * > & GetArrayOfStructs() const { ASSERT( IsArrayOfStructs() ); RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( m_SubVariables ) }

    enum VarType : uint8_t
    {
        VAR_ANY     = 0, // used for searching
        VAR_STRING  = 1,
        VAR_BOOL    = 2,
        VAR_ARRAY_OF_STRINGS = 3,
        VAR_INT     = 4,
        VAR_STRUCT  = 5,
        VAR_ARRAY_OF_STRUCTS = 6,
        MAX_VAR_TYPES    // NOTE: Be sure to update s_TypeNames when adding to here
    };

    inline VarType GetType() const  { return m_Type; }
    inline static const char * GetTypeName( VarType t ) { return s_TypeNames[ (uint32_t)t ]; }

    inline bool IsString() const    { return m_Type == BFFVariable::VAR_STRING; }
    inline bool IsBool() const      { return m_Type == BFFVariable::VAR_BOOL; }
    inline bool IsArrayOfStrings() const    { return m_Type == BFFVariable::VAR_ARRAY_OF_STRINGS; }
    inline bool IsInt() const       { return m_Type == BFFVariable::VAR_INT; }
    inline bool IsStruct() const    { return m_Type == BFFVariable::VAR_STRUCT; }
    inline bool IsArrayOfStructs() const { return m_Type == BFFVariable::VAR_ARRAY_OF_STRUCTS; }

    inline bool Frozen() const { return m_FreezeCount > 0; }
    inline void Freeze() const { ++m_FreezeCount; }
    inline void Unfreeze() const { ASSERT( m_FreezeCount != 0 ); --m_FreezeCount; }

    BFFVariable * ConcatVarsRecurse( const AString & dstName, const BFFVariable & other, const BFFToken * operatorIter ) const;

    static const BFFVariable ** GetMemberByName( const AString & name, const Array< const BFFVariable * > & members );

private:
    friend class BFFStackFrame;

    explicit BFFVariable( const BFFVariable & other );

    explicit BFFVariable( const AString & name, VarType type );
    explicit BFFVariable( const AString & name, const AString & value );
    explicit BFFVariable( const AString & name, bool value );
    explicit BFFVariable( const AString & name, const Array< AString > & values );
    explicit BFFVariable( const AString & name, int32_t i );
    explicit BFFVariable( const AString & name, const Array< const BFFVariable * > & values );
    explicit BFFVariable( const AString & name, Array<BFFVariable *> && values );
    explicit BFFVariable( const AString & name, const Array< const BFFVariable * > & structs, VarType type ); // type for disambiguation
    ~BFFVariable();

    BFFVariable & operator =( const BFFVariable & other ) = delete;

    void SetValueString( const AString & value );
    void SetValueBool( bool value );
    void SetValueArrayOfStrings( const Array< AString > & values );
    void SetValueInt( int i );
    void SetValueStruct( const Array< const BFFVariable * > & members );
    void SetValueStruct( Array<BFFVariable *> && members );
    void SetValueArrayOfStructs( const Array< const BFFVariable * > & values );

    AString m_Name;
    VarType m_Type;

    mutable uint8_t     m_FreezeCount   = 0;

    //
    bool                m_BoolValue     = false;
    int32_t             m_IntValue      = 0;
    AString             m_StringValue;
    Array< AString >    m_ArrayValues;
    Array< BFFVariable * > m_SubVariables; // Used for struct members of arrays of structs

    static const char * s_TypeNames[ MAX_VAR_TYPES ];
};

//------------------------------------------------------------------------------
