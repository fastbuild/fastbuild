// BFFVariable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Mem/Mem.h"

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
    const VarType dstType = m_Type;
    const VarType srcType = src.m_Type;

    // handle supported types

    if ( srcType != dstType )
    {
        // Mismatched - is there a supported conversion?

        const bool dstIsEmpty = ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && GetArrayOfStrings().IsEmpty() ) ||
                                ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && GetArrayOfStructs().IsEmpty() );
        const bool srcIsEmpty = ( ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && src.GetArrayOfStrings().IsEmpty() ) ||
                                ( ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && src.GetArrayOfStructs().IsEmpty() );

        switch ( srcType )
        {
            case VAR_ANY: ASSERT( false ); break;

            case VAR_STRING:
                if ( dstType == VAR_ARRAY_OF_STRINGS )
                {
                    // String to ArrayOfStrings
                    m_ArrayValues.Append( src.GetString() );
                    return true;
                }
                else if ( dstType == VAR_ARRAY_OF_STRUCTS )
                {
                    if ( dstIsEmpty )
                    {
                        // String to empty ArrayOfStructs
                        SetValueArrayOfStrings( src.GetString() );
                        return true;
                    }
                }
                break;

            case VAR_BOOL: break;

            case VAR_ARRAY_OF_STRINGS:
                if ( dstType == VAR_ARRAY_OF_STRUCTS )
                {
                    if ( srcIsEmpty )
                    {
                        // Empty ArrayOfStrings to ArrayOfStructs
                        return true;
                    }
                    if ( dstIsEmpty )
                    {
                        // ArrayOfStrings to empty ArrayOfStructs
                        SetValueArrayOfStrings( src.GetArrayOfStrings() );
                        return true;
                    }
                }
                break;

            case VAR_INT: break;

            case VAR_STRUCT:
                if ( dstType == VAR_ARRAY_OF_STRINGS )
                {
                    if ( dstIsEmpty )
                    {
                        // Struct to empty ArrayOfStrings
                        SetValueArrayOfStructs( src );
                        return true;
                    }
                }
                else if ( dstType == VAR_ARRAY_OF_STRUCTS )
                {
                    // Struct to ArrayOfStructs
                    m_SubVariables.Append( src );
                    return true;
                }
                break;

            case VAR_ARRAY_OF_STRUCTS:
                if ( dstType == VAR_ARRAY_OF_STRINGS )
                {
                    if ( srcIsEmpty )
                    {
                        // Empty ArrayOfStructs to ArrayOfStrings
                        return true;
                    }
                    if ( dstIsEmpty )
                    {
                        // ArrayOfStructs to empty ArrayOfStrings
                        SetValueArrayOfStructs( src.GetArrayOfStructs() );
                        return true;
                    }
                }
                break;

            case MAX_VAR_TYPES: ASSERT( false ); break;
        }

        // Incompatible types
        Error::Error_1034_OperationNotSupported( operatorIter, // TODO:C we don't have access to the rhsIterator so we use the operator
                                                 dstType,
                                                 srcType,
                                                 operatorIter );
        return false;
    }

    // Matching Src and Dst

    switch ( srcType )
    {
        case VAR_ANY: ASSERT( false ); return false;

        case VAR_STRING:
            m_StringValue.Append( src.GetString() );
            return true;

        case VAR_BOOL:
            // Assume + <=> OR
            m_BoolValue |= src.GetBool();
            return true;

        case VAR_ARRAY_OF_STRINGS:
            m_ArrayValues.Append( src.GetArrayOfStrings() );
            return true;

        case VAR_INT:
            m_IntValue += src.GetInt();
            return true;

        case VAR_STRUCT:
        {
            for ( const BFFVariable & srcMember : src.GetStructMembers() )
            {
                if ( BFFVariable * dstMember = GetMemberByName( srcMember.GetName(), m_SubVariables ) )
                {
                    // keep original (dst) members where member is only present in original (dst)
                    // or concatenate recursively members where the name exists in both
                    if ( dstMember->Concat( srcMember, operatorIter ) == false )
                    {
                        // ConcatVarsRecurse will have emitted an error
                        return false;
                    }
                }
                else
                {
                    // and add members only present in the src
                    m_SubVariables.Append( srcMember );
                }
            }
            return true;
        }

        case VAR_ARRAY_OF_STRUCTS:
            m_SubVariables.Append( src.GetArrayOfStructs() );
            return true;

        case MAX_VAR_TYPES: ASSERT( false ); return false;
    }

    ASSERT( false ); // Should never get here
    return false;
}

//------------------------------------------------------------------------------
