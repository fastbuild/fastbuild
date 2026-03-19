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
        case VAR_STRING: SetValueString( other.GetString() ); break;
        case VAR_BOOL: SetValueBool( other.GetBool() ); break;
        case VAR_ARRAY_OF_STRINGS: SetValueArrayOfStrings( other.GetArrayOfStrings() ); break;
        case VAR_INT: SetValueInt( other.GetInt() ); break;
        case VAR_STRUCT: SetValueStruct( other.GetStructMembers() ); break;
        case VAR_ARRAY_OF_STRUCTS: SetValueArrayOfStructs( other.GetArrayOfStructs() ); break;
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
        case VAR_BOOL: SetValueBool( other.GetBool() ); break;
        case VAR_ARRAY_OF_STRINGS: m_ArrayValues = Move( other.m_ArrayValues ); break;
        case VAR_INT: SetValueInt( other.GetInt() ); break;
        case VAR_STRUCT: m_SubVariables = Move( other.m_SubVariables ); break;
        case VAR_ARRAY_OF_STRUCTS: m_SubVariables = Move( other.m_SubVariables ); break;
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
                          const Array<BFFVariable> & values )
    : m_Name( name )
    , m_Type( VAR_STRUCT )
    , m_SubVariables( values )
    , m_Token( token )
{
}

// CONSTRUCTOR (&&)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          Array<BFFVariable> && values )
    : m_Name( name )
    , m_Type( VAR_STRUCT )
    , m_SubVariables( Move( values ) )
    , m_Token( token )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          const Array<BFFVariable> & structs,
                          VarType type ) // type for disambiguation
    : m_Name( name )
    , m_Type( VAR_ARRAY_OF_STRUCTS )
    , m_SubVariables( structs )
    , m_Token( token )
{
    // type for disambiguation only - sanity check it's the right type
    ASSERT( type == VAR_ARRAY_OF_STRUCTS );
    (void)type;
}

// SetValueString
//------------------------------------------------------------------------------
void BFFVariable::SetValueString( const AString & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRING );
    m_StringValue = value;
}

// SetValueBool
//------------------------------------------------------------------------------
void BFFVariable::SetValueBool( bool value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_BOOL );
    m_BoolValue = value;
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( const Array<AString> & values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues = values;
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( Array<AString> && values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues = Move( values );
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( const AString & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRINGS );
    m_ArrayValues.Clear();
    m_ArrayValues.Append( value );
}

// SetValueInt
//------------------------------------------------------------------------------
void BFFVariable::SetValueInt( int i )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_INT );
    m_IntValue = i;
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( const Array<BFFVariable> & values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRUCT );
    m_SubVariables = values;
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( Array<BFFVariable> && values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_STRUCT );
    m_SubVariables = Move( values );
}

// SetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStructs( const Array<BFFVariable> & values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_SubVariables = values;
}

// SetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStructs( Array<BFFVariable> && values )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_SubVariables = Move( values );
}

// SetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStructs( const BFFVariable & value )
{
    ASSERT( 0 == m_FreezeCount );
    SetType( VAR_ARRAY_OF_STRUCTS );
    m_SubVariables.Clear();
    m_SubVariables.Append( value );
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
        case VAR_STRUCT:
            if ( type == VAR_ARRAY_OF_STRUCTS )
            {
                // Don't release the memory just yet in case it is about to be reused.
                m_SubVariables.Clear();
            }
            else
            {
                m_SubVariables.Destruct();
            }
            break;
        case VAR_ARRAY_OF_STRUCTS:
            if ( type == VAR_STRUCT )
            {
                // Don't release the memory just yet in case it is about to be reused.
                m_SubVariables.Clear();
            }
            else
            {
                m_SubVariables.Destruct();
            }
            break;
        case MAX_VAR_TYPES: ASSERT( false ); break;
    }
    m_Type = type;
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFVariable::GetMemberByName( const AString & name, const Array<BFFVariable> & members )
{
    ASSERT( !name.IsEmpty() );

    for ( const BFFVariable * it = members.Begin(); it != members.End(); ++it )
    {
        if ( it->GetName() == name )
        {
            return it;
        }
    }

    return nullptr;
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFVariable::GetMemberByName( const AString & name, Array<BFFVariable> & members )
{
    ASSERT( !name.IsEmpty() );

    for ( BFFVariable * it = members.Begin(); it != members.End(); ++it )
    {
        if ( it->GetName() == name )
        {
            return it;
        }
    }

    return nullptr;
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
        case VAR_STRUCT: return ConcatValue<BFFVariable::VAR_STRUCT, const Array<BFFVariable> &>( src.GetStructMembers(), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, const Array<BFFVariable> &>( src.GetArrayOfStructs(), operatorIter );
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
        case VAR_STRUCT: return ConcatValue<BFFVariable::VAR_STRUCT, Array<BFFVariable> &&>( Move( src.m_SubVariables ), operatorIter );
        case VAR_ARRAY_OF_STRUCTS: return ConcatValue<BFFVariable::VAR_ARRAY_OF_STRUCTS, Array<BFFVariable> &&>( Move( src.m_SubVariables ), operatorIter );
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
                SetValueArrayOfStrings( Forward( V, srcValue ) );
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
                    SetValueArrayOfStrings( Forward( V, srcValue ) );
                    return true;
                }
            }
        }
        else if constexpr ( srcType == VAR_STRUCT )
        {
            if ( dstType == VAR_ARRAY_OF_STRINGS && dstIsEmpty )
            {
                // Struct to empty ArrayOfStrings
                SetValueArrayOfStructs( Forward( V, srcValue ) );
                return true;
            }
            else if ( dstType == VAR_ARRAY_OF_STRUCTS )
            {
                // Struct to ArrayOfStructs
                m_SubVariables.Append( Forward( V, srcValue ) );
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
                    SetValueArrayOfStructs( Forward( V, srcValue ) );
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
            if ( BFFVariable * dstMember = GetMemberByName( srcMember.GetName(), m_SubVariables ) )
            {
                // keep original (dst) members where member is only present in original (dst)
                // or concatenate recursively members where the name exists in both
                const bool result = std::is_rvalue_reference_v<V> ?
                                    dstMember->Concat( Move( srcMember ), operatorIter ) :
                                    dstMember->Concat( srcMember, operatorIter );
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
                    m_SubVariables.Append( Move( srcMember ) );
                }
                else
                {
                    m_SubVariables.Append( srcMember );
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
        m_SubVariables.Append( Forward( V, srcValue ) );
        return true;
    }
    else
    {
        static_assert( sizeof( srcType ) == 0, "Unsupported type" );
    }

    ASSERT( false ); // Should never get here
    return false;
}

//------------------------------------------------------------------------------
