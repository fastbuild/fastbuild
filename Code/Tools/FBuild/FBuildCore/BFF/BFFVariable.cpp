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
                          const Array<const BFFVariable *> & values )
    : m_Name( name )
    , m_Type( VAR_STRUCT )
    , m_Token( token )
{
    m_SubVariables.SetCapacity( values.GetSize() );
    SetValueStruct( values );
}

// CONSTRUCTOR (&&)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const BFFToken & token,
                          Array<BFFVariable *> && values )
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
                          const Array<const BFFVariable *> & structs,
                          VarType type ) // type for disambiguation
    : m_Name( name )
    , m_Type( VAR_ARRAY_OF_STRUCTS )
    , m_Token( token )
{
    m_SubVariables.SetCapacity( structs.GetSize() );

    // type for disambiguation only - sanity check it's the right type
    ASSERT( type == VAR_ARRAY_OF_STRUCTS );
    (void)type;

    SetValueArrayOfStructs( structs );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::~BFFVariable()
{
    // clean up sub variables
    for ( BFFVariable * var : m_SubVariables )
    {
        FDELETE var;
    }
}

// SetValueString
//------------------------------------------------------------------------------
void BFFVariable::SetValueString( const AString & value )
{
    ASSERT( 0 == m_FreezeCount );
    m_Type = VAR_STRING;
    m_StringValue = value;
}

// SetValueBool
//------------------------------------------------------------------------------
void BFFVariable::SetValueBool( bool value )
{
    ASSERT( 0 == m_FreezeCount );
    m_Type = VAR_BOOL;
    m_BoolValue = value;
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( const Array<AString> & values )
{
    ASSERT( 0 == m_FreezeCount );
    m_Type = VAR_ARRAY_OF_STRINGS;
    m_ArrayValues = values;
}

// SetValueInt
//------------------------------------------------------------------------------
void BFFVariable::SetValueInt( int i )
{
    ASSERT( 0 == m_FreezeCount );
    m_Type = VAR_INT;
    m_IntValue = i;
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( const Array<const BFFVariable *> & values )
{
    ASSERT( 0 == m_FreezeCount );

    // build list of new members, but don't touch old ones yet to gracefully
    // handle self-assignment
    Array<BFFVariable *> newVars;
    newVars.SetCapacity( values.GetSize() );

    m_Type = VAR_STRUCT;
    for ( const BFFVariable * var : values )
    {
        newVars.Append( FNEW( BFFVariable( *var ) ) );
    }

    // free old members
    for ( BFFVariable * var : m_SubVariables )
    {
        FDELETE var;
    }

    // swap
    m_SubVariables.Swap( newVars );
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( Array<BFFVariable *> && values )
{
    ASSERT( 0 == m_FreezeCount );

    // Take a copy of the old pointers
    Array<BFFVariable *> oldVars;
    oldVars.Swap( m_SubVariables );

    // Take ownership of new variables
    m_SubVariables = Move( values );

    // Free old variables
    for ( BFFVariable * var : oldVars )
    {
        FDELETE var;
    }
}

// SetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStructs( const Array<const BFFVariable *> & values )
{
    ASSERT( 0 == m_FreezeCount );

    // build list of new members, but don't touch old ones yet to gracefully
    // handle self-assignment
    Array<BFFVariable *> newVars;
    newVars.SetCapacity( values.GetSize() );

    m_Type = VAR_ARRAY_OF_STRUCTS;
    for ( const BFFVariable * var : values )
    {
        newVars.Append( FNEW( BFFVariable( *var ) ) );
    }

    // free old members
    for ( BFFVariable * var : m_SubVariables )
    {
        FDELETE var;
    }

    m_SubVariables.Swap( newVars );
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ const BFFVariable ** BFFVariable::GetMemberByName( const AString & name, const Array<const BFFVariable *> & members )
{
    ASSERT( !name.IsEmpty() );

    for ( const BFFVariable ** it = members.Begin(); it != members.End(); ++it )
    {
        if ( ( *it )->GetName() == name )
        {
            return it;
        }
    }

    return nullptr;
}

// ConcatVarsRecurse
//------------------------------------------------------------------------------
BFFVariable * BFFVariable::ConcatVarsRecurse( const AString & dstName, const BFFVariable & other, const BFFToken * operatorIter ) const
{
    const BFFVariable * const varDst = this;
    const BFFVariable * const varSrc = &other;

    const VarType dstType = m_Type;
    const VarType srcType = other.m_Type;

    // handle supported types

    if ( srcType != dstType )
    {
        // Mismatched - is there a supported conversion?

        const bool dstIsEmpty = ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && varDst->GetArrayOfStrings().IsEmpty() ) ||
                                ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && varDst->GetArrayOfStructs().IsEmpty() );
        const bool srcIsEmpty = ( ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && varSrc->GetArrayOfStrings().IsEmpty() ) ||
                                ( ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && varSrc->GetArrayOfStructs().IsEmpty() );

        // String to ArrayOfStrings or empty array
        if ( ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) || dstIsEmpty ) &&
             ( srcType == BFFVariable::VAR_STRING ) )
        {
            StackArray<AString> values;
            values.SetCapacity( varDst->GetArrayOfStrings().GetSize() + 1 );
            if ( !dstIsEmpty )
            {
                values.Append( varDst->GetArrayOfStrings() );
            }
            values.Append( varSrc->GetString() );
            BFFVariable * result = FNEW( BFFVariable( dstName, m_Token, values ) );
            return result;
        }
        // Struct to ArrayOfStructs or empty array concatenation
        if ( ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) || dstIsEmpty ) &&
             ( srcType == BFFVariable::VAR_STRUCT ) )
        {
            const uint32_t num = (uint32_t)( 1 + ( !dstIsEmpty ? varDst->GetArrayOfStructs().GetSize() : 0 ) );
            StackArray<const BFFVariable *> values;
            values.SetCapacity( num );
            if ( !dstIsEmpty )
            {
                values.Append( varDst->GetArrayOfStructs() );
            }
            values.Append( varSrc );

            BFFVariable * result = FNEW( BFFVariable( dstName, m_Token, values, VAR_ARRAY_OF_STRUCTS ) );
            return result;
        }

        // Empty array to Array of any type or vice-versa
        if ( ( ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) || ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) ) && srcIsEmpty ) ||
             ( ( ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) || ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS ) ) && dstIsEmpty ) )
        {
            const BFFVariable * src = srcIsEmpty ? varDst : varSrc;
            BFFVariable * result = FNEW( BFFVariable( dstName, m_Token, src->m_Type ) );
            if ( src->m_Type == BFFVariable::VAR_ARRAY_OF_STRINGS )
            {
                result->SetValueArrayOfStrings( src->GetArrayOfStrings() );
            }
            else
            {
                result->SetValueArrayOfStructs( src->GetArrayOfStructs() );
            }
            return result;
        }

        // Incompatible types
        Error::Error_1034_OperationNotSupported( operatorIter, // TODO:C we don't have access to the rhsIterator so we use the operator
                                                 varDst->GetType(),
                                                 varSrc->GetType(),
                                                 operatorIter );
        return nullptr;
    }
    else
    {
        // Matching Src and Dst

        if ( srcType == BFFVariable::VAR_STRING )
        {
            AStackString<2048> finalValue;
            finalValue = varDst->GetString();
            finalValue += varSrc->GetString();

            BFFVariable * result = FNEW( BFFVariable( dstName, varSrc->m_Token, finalValue ) );
            return result;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS )
        {
            const unsigned int num = (unsigned int)( varSrc->GetArrayOfStrings().GetSize() + varDst->GetArrayOfStrings().GetSize() );
            StackArray<AString> values;
            values.SetCapacity( num );
            values.Append( varDst->GetArrayOfStrings() );
            values.Append( varSrc->GetArrayOfStrings() );

            BFFVariable * result = FNEW( BFFVariable( dstName, varSrc->m_Token, values ) );
            return result;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS )
        {
            const unsigned int num = (unsigned int)( varSrc->GetArrayOfStructs().GetSize() + varDst->GetArrayOfStructs().GetSize() );
            StackArray<const BFFVariable *> values;
            values.SetCapacity( num );
            values.Append( varDst->GetArrayOfStructs() );
            values.Append( varSrc->GetArrayOfStructs() );

            BFFVariable * result = FNEW( BFFVariable( dstName, varSrc->m_Token, values, VAR_ARRAY_OF_STRUCTS ) );
            return result;
        }

        if ( srcType == BFFVariable::VAR_INT )
        {
            int newVal( varSrc->GetInt() );
            newVal += varDst->GetInt();

            BFFVariable * result = FNEW( BFFVariable( dstName, varSrc->m_Token, newVal ) );
            return result;
        }

        if ( srcType == BFFVariable::VAR_BOOL )
        {
            // Assume + <=> OR
            bool newVal( varSrc->GetBool() );
            newVal |= varDst->GetBool();

            BFFVariable * result = FNEW( BFFVariable( dstName, varSrc->m_Token, newVal ) );
            return result;
        }

        if ( srcType == BFFVariable::VAR_STRUCT )
        {
            const Array<const BFFVariable *> & srcMembers = varSrc->GetStructMembers();
            const Array<const BFFVariable *> & dstMembers = varDst->GetStructMembers();

            BFFVariable * const result = FNEW( BFFVariable( dstName, varSrc->m_Token, BFFVariable::VAR_STRUCT ) );
            result->m_SubVariables.SetCapacity( srcMembers.GetSize() + dstMembers.GetSize() );
            Array<BFFVariable *> & allMembers = result->m_SubVariables;

            // keep original (dst) members where member is only present in original (dst)
            // or concatenate recursively members where the name exists in both
            for ( const BFFVariable ** it = dstMembers.Begin(); it != dstMembers.End(); ++it )
            {
                const BFFVariable * const * it2 = GetMemberByName( ( *it )->GetName(), srcMembers );

                BFFVariable * newVar;
                if ( it2 )
                {
                    newVar = ( *it )->ConcatVarsRecurse( ( *it )->GetName(), **it2, operatorIter );
                    if ( newVar == nullptr )
                    {
                        FDELETE result;
                        return nullptr; // ConcatVarsRecurse will have emitted an error
                    }
                }
                else
                {
                    newVar = FNEW( BFFVariable( **it ) );
                }

                allMembers.Append( newVar );
            }

            // and add members only present in the src
            for ( const BFFVariable ** it = srcMembers.Begin(); it != srcMembers.End(); ++it )
            {
                const BFFVariable * const * it2 = GetMemberByName( ( *it )->GetName(), result->GetStructMembers() );
                if ( nullptr == it2 )
                {
                    BFFVariable * const newVar = FNEW( BFFVariable( **it ) );
                    allMembers.Append( newVar );
                }
            }

            return result;
        }
    }

    ASSERT( false ); // Should never get here
    return nullptr;
}

//------------------------------------------------------------------------------
