// BFFVariable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Mem/Mem.h"

#include <type_traits>

// Static Data
//------------------------------------------------------------------------------
// clang-format off
/*static*/ const char * BFFVariable::s_TypeNames[] =
{
    "Any",
    "String",
    "Bool",
    "ArrayOfStrings",
    "Int",
    "Struct",
    "ArrayOfStructs"
};
// clang-format on

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const BFFToken & token, VarType type )
    : m_Name( name )
    , m_Type( type )
    , m_Token( token )
{
}

// CONSTRUCTOR (copy)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const BFFVariable & other )
    : m_Name( other.m_Name )
    , m_Type( other.m_Type )
    , m_Token( other.m_Token )
{
    switch ( m_Type )
    {
        case VAR_ANY: ASSERT( false ); break;
        case VAR_STRING: ForceSetValueString( other.GetString() ); break;
        case VAR_BOOL: ForceSetValueBool( other.GetBool() ); break;
        case VAR_ARRAY_OF_STRINGS: ForceSetValueArrayOfStrings( other.GetArrayOfStrings() ); break;
        case VAR_INT: ForceSetValueInt( other.GetInt() ); break;
        case VAR_STRUCT: ForceSetValueStruct( other.GetStruct() ); break;
        case VAR_ARRAY_OF_STRUCTS: ForceSetValueArrayOfStructs( other.GetArrayOfStructs() ); break;
        case MAX_VAR_TYPES: ASSERT( false ); break;
    }
}

// CONSTRUCTOR (move)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( BFFVariable && other )
    : m_Name( Move( other.m_Name ) )
    , m_Type( other.m_Type )
    , m_Token( other.m_Token )
{
    switch ( m_Type )
    {
        case VAR_ANY: ASSERT( false ); break;
        case VAR_STRING: m_StringValue = Move( other.m_StringValue ); break;
        case VAR_BOOL: ForceSetValueBool( other.GetBool() ); break;
        case VAR_ARRAY_OF_STRINGS: m_ArrayValues = Move( other.m_ArrayValues ); break;
        case VAR_INT: ForceSetValueInt( other.GetInt() ); break;
        case VAR_STRUCT: m_StructValue = Move( other.m_StructValue ); break;
        case VAR_ARRAY_OF_STRUCTS: m_StructArrayValues = Move( other.m_StructArrayValues ); break;
        case MAX_VAR_TYPES: ASSERT( false ); break;
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          const AString & value )
    : m_Name( name )
    , m_Type( VAR_STRING )
    , m_StringValue( value )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          bool value )
    : m_Name( name )
    , m_Type( VAR_BOOL )
    , m_BoolValue( value )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          const Array<AString> & values )
    : m_Name( name )
    , m_Type( VAR_ARRAY_OF_STRINGS )
    , m_ArrayValues( values )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          int32_t i )
    : m_Name( name )
    , m_Type( VAR_INT )
    , m_IntValue( i )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          const BFFVariableScope & value )
    : m_Name( name )
    , m_Type( VAR_STRUCT )
    , m_StructValue( value )
    , m_Token( token )
{
}

// CONSTRUCTOR (&&)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          BFFVariableScope && value )
    : m_Name( name )
    , m_Type( VAR_STRUCT )
    , m_StructValue( Move( value ) )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          const Array<BFFVariableScope> & structs )
    : m_Name( name )
    , m_Type( VAR_ARRAY_OF_STRUCTS )
    , m_StructArrayValues( structs )
    , m_Token( token )
{
}

// ForceSetValueString
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueString( const AString & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRING );
    m_StringValue = value;
}

// ForceSetValueString
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueString( AString && value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRING );
    m_StringValue = Move( value );
}

// ForceSetValueBool
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueBool( bool value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_BOOL );
    m_BoolValue = value;
}

// ForceSetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStrings( const Array<AString> & values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues = values;
}

// ForceSetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStrings( Array<AString> && values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues = Move( values );
}

// ForceSetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStrings( const AString & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues.Clear();
    m_ArrayValues.Append( value );
}

// ForceSetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStrings( AString && value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues.Clear();
    m_ArrayValues.Append( Move( value ) );
}

// ForceSetValueInt
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueInt( int i )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_INT );
    m_IntValue = i;
}

// ForceSetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueStruct( const BFFVariableScope & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRUCT );
    m_StructValue = value;
}

// ForceSetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueStruct( BFFVariableScope && value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRUCT );
    m_StructValue = Move( value );
}

// ForceSetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStructs( const Array<BFFVariableScope> & values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_StructArrayValues = values;
}

// ForceSetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStructs( Array<BFFVariableScope> && values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_StructArrayValues = Move( values );
}

// ForceSetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStructs( const BFFVariableScope & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_StructArrayValues.Clear();
    m_StructArrayValues.Append( value );
}

// ForceSetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::ForceSetValueArrayOfStructs( BFFVariableScope && value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_StructArrayValues.Clear();
    m_StructArrayValues.Append( Move( value ) );
}

// SetType
//------------------------------------------------------------------------------
void BFFVariable::SetType( VarType type )
{
    if ( type == m_Type )
    {
        return;
    }
    switch ( m_Type )
    {
        case VAR_ANY: break;
        case VAR_STRING: m_StringValue.ClearAndFreeMemory(); break;
        case VAR_BOOL: break;
        case VAR_ARRAY_OF_STRINGS: m_ArrayValues.Destruct(); break;
        case VAR_INT: break;
        case VAR_STRUCT: m_StructValue = BFFVariableScope(); break;
        case VAR_ARRAY_OF_STRUCTS: m_StructArrayValues.Destruct(); break;
        case MAX_VAR_TYPES: ASSERT( false ); break;
    }
    m_Type = type;
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFVariable::GetMemberByName( const AString & name, const BFFVariableScope & members )
{
    ASSERT( !name.IsEmpty() );

    if ( const BFFVariable * var = members.Find( name ) )
    {
        ASSERT( var->GetName() == name );
        return var;
    }

    return nullptr;
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFVariable::GetMemberByName( const AString & name, BFFVariableScope & members )
{
    ASSERT( !name.IsEmpty() );

    if ( BFFVariable * var = members.Find( name ) )
    {
        ASSERT( var->GetName() == name );
        return var;
    }

    return nullptr;
}

// Set
//------------------------------------------------------------------------------
bool BFFVariable::Set( const BFFVariable & src, const BFFToken * operatorIter )
{
    switch ( src.GetType() )
    {
        case VAR_ANY: ASSERT( false ); return false;
        case VAR_STRING: return SetValue<BFFVariable::VAR_STRING, const AString &>( src.GetString(), operatorIter );
        case VAR_BOOL: return SetValue<BFFVariable::VAR_BOOL, bool>( src.GetBool(), operatorIter );
        case VAR_ARRAY_OF_STRINGS: return SetValue<BFFVariable::VAR_ARRAY_OF_STRINGS, const Array<AString> &>( src.GetArrayOfStrings(), operatorIter );
        case VAR_INT: return SetValue<BFFVariable::VAR_INT, int32_t>( src.GetInt(), operatorIter );
        case VAR_STRUCT: return SetValue<BFFVariable::VAR_STRUCT, const BFFVariableScope &>( src.GetStruct(), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return SetValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, const Array<BFFVariableScope> &>( src.GetArrayOfStructs(), operatorIter );
        case MAX_VAR_TYPES: ASSERT( false ); return false;
    }

    ASSERT( false );
    return false;
}

// SetValue
//------------------------------------------------------------------------------
template <BFFVariable::VarType SrcType, class V>
bool BFFVariable::SetValue( V value, const BFFToken * operatorIter )
{
    const VarType dstType = m_Type;
    constexpr VarType srcType = SrcType;

    const bool dstIsEmpty = ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && GetArrayOfStrings().IsEmpty() ) ||
                            ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && GetArrayOfStructs().IsEmpty() );

    // make sure types are compatible
    if constexpr ( srcType == VAR_STRING )
    {
        if ( dstType == VAR_STRING )
        {
            // OK - assigning to a new variable or to a string
            ForceSetValueString( Forward( V, value ) );
            return true;
        }
        else if ( dstType == VAR_ARRAY_OF_STRINGS || dstIsEmpty )
        {
            // OK - store new string as the single element of array
            ForceSetValueArrayOfStrings( Forward( V, value ) );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_BOOL )
    {
        if ( dstType == VAR_BOOL )
        {
            ForceSetValueBool( Forward( V, value ) );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_ARRAY_OF_STRINGS )
    {
        if ( dstType == VAR_ARRAY_OF_STRINGS || dstIsEmpty )
        {
            ForceSetValueArrayOfStrings( Forward( V, value ) );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_INT )
    {
        if ( dstType == VAR_INT )
        {
            ForceSetValueInt( Forward( V, value ) );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_STRUCT )
    {
        if ( dstType == VAR_STRUCT )
        {
            ForceSetValueStruct( Forward( V, value ) );
            return true;
        }
        else if ( dstType == VAR_ARRAY_OF_STRUCTS || dstIsEmpty )
        {
            // OK - store struct as the single element of array
            ForceSetValueArrayOfStructs( Forward( V, value ) );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_ARRAY_OF_STRUCTS )
    {
        if ( dstType == VAR_ARRAY_OF_STRUCTS || dstIsEmpty )
        {
            ForceSetValueArrayOfStructs( Forward( V, value ) );
            return true;
        }
    }

    Error::Error_1034_OperationNotSupported( operatorIter, dstType, srcType, operatorIter );
    return false;
}

// Concat
//------------------------------------------------------------------------------
bool BFFVariable::Concat( const BFFVariable & src, const BFFToken * operatorIter )
{
    switch ( src.GetType() )
    {
        case VAR_ANY: ASSERT( false ); return false;
        case VAR_STRING: return ConcatValue<BFFVariable::VAR_STRING, const AString &>( src.GetString(), operatorIter );
        case VAR_BOOL: return ConcatValue<BFFVariable::VAR_BOOL, bool>( src.GetBool(), operatorIter );
        case VAR_ARRAY_OF_STRINGS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRINGS, const Array<AString> &>( src.GetArrayOfStrings(), operatorIter );
        case VAR_INT: return ConcatValue<BFFVariable::VAR_INT, int32_t>( src.GetInt(), operatorIter );
        case VAR_STRUCT: return ConcatValue<BFFVariable::VAR_STRUCT, const BFFVariableScope &>( src.GetStruct(), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, const Array<BFFVariableScope> &>( src.GetArrayOfStructs(), operatorIter );
        case MAX_VAR_TYPES: ASSERT( false ); return false;
    }

    ASSERT( false );
    return false;
}

// Concat
//------------------------------------------------------------------------------
bool BFFVariable::Concat( BFFVariable && src, const BFFToken * operatorIter )
{
    switch ( src.GetType() )
    {
        case VAR_ANY: ASSERT( false ); return false;
        case VAR_STRING: return ConcatValue<BFFVariable::VAR_STRING, AString &&>( Move( src.m_StringValue ), operatorIter );
        case VAR_BOOL: return ConcatValue<BFFVariable::VAR_BOOL, bool>( src.GetBool(), operatorIter );
        case VAR_ARRAY_OF_STRINGS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRINGS, Array<AString> &&>( Move( src.m_ArrayValues ), operatorIter );
        case VAR_INT: return ConcatValue<BFFVariable::VAR_INT, int32_t>( src.GetInt(), operatorIter );
        case VAR_STRUCT: return ConcatValue<BFFVariable::VAR_STRUCT, BFFVariableScope &&>( Move( src.m_StructValue ), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, Array<BFFVariableScope> &&>( Move( src.m_StructArrayValues ), operatorIter );
        case MAX_VAR_TYPES: ASSERT( false ); return false;
    }

    ASSERT( false );
    return false;
}

// ConcatValue
//------------------------------------------------------------------------------
template <BFFVariable::VarType SrcType, class V>
bool BFFVariable::ConcatValue( V srcValue, const BFFToken * operatorIter )
{
    static_assert( ( std::is_reference_v<V> && std::is_const_v<std::remove_reference_t<V>> ) ||
                   std::is_integral_v<V> ||
                   std::is_rvalue_reference_v<V> );

    const VarType dstType = m_Type;
    constexpr VarType srcType = SrcType;

    // handle supported types

    if ( srcType != dstType )
    {
        // Mismatched - is there a supported conversion?

        const bool dstIsEmpty = ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && GetArrayOfStrings().IsEmpty() ) ||
                                ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && GetArrayOfStructs().IsEmpty() );

        if constexpr ( srcType == VAR_STRING )
        {
            if ( dstType == VAR_ARRAY_OF_STRINGS )
            {
                // String to ArrayOfStrings
                m_ArrayValues.Append( Forward( V, srcValue ) );
                return true;
            }
            else if ( dstType == VAR_ARRAY_OF_STRUCTS && dstIsEmpty )
            {
                // String to empty ArrayOfStructs
                ForceSetValueArrayOfStrings( Forward( V, srcValue ) );
                return true;
            }
        }
        else if constexpr ( srcType == VAR_ARRAY_OF_STRINGS )
        {
            if ( dstType == VAR_ARRAY_OF_STRUCTS )
            {
                if ( srcValue.IsEmpty() )
                {
                    // Empty ArrayOfStrings to ArrayOfStructs
                    return true;
                }
                if ( dstIsEmpty )
                {
                    // ArrayOfStrings to empty ArrayOfStructs
                    ForceSetValueArrayOfStrings( Forward( V, srcValue ) );
                    return true;
                }
            }
        }
        else if constexpr ( srcType == VAR_STRUCT )
        {
            if ( dstType == VAR_ARRAY_OF_STRINGS && dstIsEmpty )
            {
                // Struct to empty ArrayOfStrings
                ForceSetValueArrayOfStructs( Forward( V, srcValue ) );
                return true;
            }
            else if ( dstType == VAR_ARRAY_OF_STRUCTS )
            {
                // Struct to ArrayOfStructs
                m_StructArrayValues.Append( Forward( V, srcValue ) );
                return true;
            }
        }
        else if constexpr ( srcType == VAR_ARRAY_OF_STRUCTS )
        {
            if ( dstType == VAR_ARRAY_OF_STRINGS )
            {
                if ( srcValue.IsEmpty() )
                {
                    // Empty ArrayOfStructs to ArrayOfStrings
                    return true;
                }
                if ( dstIsEmpty )
                {
                    // ArrayOfStructs to empty ArrayOfStrings
                    ForceSetValueArrayOfStructs( Forward( V, srcValue ) );
                    return true;
                }
            }
        }

        // Incompatible types
        Error::Error_1034_OperationNotSupported( operatorIter, // TODO:C we don't have access to the rhsIterator so we use the operator
                                                 dstType,
                                                 srcType,
                                                 operatorIter );
        return false;
    }

    // Matching Src and Dst

    ASSERT( srcType == dstType );

    if constexpr ( srcType == VAR_STRING )
    {
        m_StringValue.Append( Forward( V, srcValue ) );
        return true;
    }
    else if constexpr ( srcType == VAR_BOOL )
    {
        m_BoolValue |= srcValue;
        return true;
    }
    else if constexpr ( srcType == VAR_ARRAY_OF_STRINGS )
    {
        m_ArrayValues.Append( Forward( V, srcValue ) );
        return true;
    }
    else if constexpr ( srcType == VAR_INT )
    {
        m_IntValue += srcValue;
        return true;
    }
    else if constexpr ( srcType == VAR_STRUCT )
    {
        for ( auto & srcMember : srcValue )
        {
            if ( auto * dstMember = GetMemberByName( srcMember.GetName(), m_StructValue ) )
            {
                // keep original (dst) members where member is only present in original (dst)
                // or concatenate recursively members where the name exists in both
                bool result;
                if constexpr ( std::is_rvalue_reference_v<V> )
                {
                    result = dstMember->Concat( Move( srcMember ), operatorIter );
                }
                else
                {
                    result = dstMember->Concat( srcMember, operatorIter );
                }
                if ( result == false )
                {
                    // Concat will have emitted an error
                    return false;
                }
            }
            else
            {
                // and add members only present in the src
                if constexpr ( std::is_rvalue_reference_v<V> )
                {
                    m_StructValue.Emplace( srcMember.GetName(), Move( srcMember ) );
                }
                else
                {
                    m_StructValue.Emplace( srcMember.GetName(), srcMember );
                }
            }
        }
        if constexpr ( std::is_rvalue_reference_v<V> )
        {
            srcValue.Clear();
        }
        return true;
    }
    else if constexpr ( srcType == VAR_ARRAY_OF_STRUCTS )
    {
        m_StructArrayValues.Append( Forward( V, srcValue ) );
        return true;
    }
    else
    {
        static_assert( sizeof( srcType ) == 0, "Unsupported type" );
    }

    ASSERT( false ); // Should never get here
    return false;
}

// Subtract
//------------------------------------------------------------------------------
bool BFFVariable::Subtract( const BFFVariable & src, const BFFToken * operatorIter )
{
    switch ( src.GetType() )
    {
        case VAR_ANY: ASSERT( false ); return false;
        case VAR_STRING: return SubtractValue<BFFVariable::VAR_STRING, const AString &>( src.GetString(), operatorIter );
        case VAR_BOOL: return SubtractValue<BFFVariable::VAR_BOOL, bool>( src.GetBool(), operatorIter );
        case VAR_ARRAY_OF_STRINGS: return SubtractValue<BFFVariable::VAR_ARRAY_OF_STRINGS, const Array<AString> &>( src.GetArrayOfStrings(), operatorIter );
        case VAR_INT: return SubtractValue<BFFVariable::VAR_INT, int32_t>( src.GetInt(), operatorIter );
        case VAR_STRUCT: return SubtractValue<BFFVariable::VAR_STRUCT, const BFFVariableScope &>( src.GetStruct(), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return SubtractValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, const Array<BFFVariableScope> &>( src.GetArrayOfStructs(), operatorIter );
        case MAX_VAR_TYPES: ASSERT( false ); return false;
    }

    ASSERT( false );
    return false;
}

// SubtractValue
//------------------------------------------------------------------------------
template <BFFVariable::VarType SrcType, class V>
bool BFFVariable::SubtractValue( const V & value, const BFFToken * operatorIter )
{
    const VarType dstType = m_Type;
    constexpr VarType srcType = SrcType;

    if constexpr ( srcType == VAR_STRING )
    {
        if ( dstType == VAR_STRING )
        {
            m_StringValue.Replace( value.Get(), "" );
            return true;
        }
        else if ( dstType == VAR_ARRAY_OF_STRINGS )
        {
            m_ArrayValues.FindAndEraseAll( value );
            return true;
        }
    }
    else if constexpr ( srcType == VAR_ARRAY_OF_STRINGS || srcType == VAR_ARRAY_OF_STRUCTS )
    {
        // Only allow subtraction of arrays if the subtracted value is empty (the no-op case).
        if ( ( dstType == VAR_ARRAY_OF_STRINGS || dstType == VAR_ARRAY_OF_STRUCTS ) && value.IsEmpty() )
        {
            return true;
        }
    }
    else if constexpr ( srcType == VAR_INT )
    {
        if ( dstType == VAR_INT )
        {
            m_IntValue -= value;
            return true;
        }
    }

    Error::Error_1034_OperationNotSupported( operatorIter,
                                             dstType,
                                             srcType,
                                             operatorIter );
    return false;
}

//------------------------------------------------------------------------------
