// BFFVariable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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
BFFVariable::BFFVariable( const AString & name, VarType type )
: m_Name( name )
, m_Type( type )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, true )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, true )
{
}

// CONSTRUCTOR (copy)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const BFFVariable & other )
: m_Name( other.m_Name )
, m_Type( other.m_Type )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, true )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, true )
{
    switch( m_Type )
    {
        case VAR_ANY:               ASSERT( false ); break;
        case VAR_STRING:            SetValueString( other.GetString() ); break;
        case VAR_BOOL:              SetValueBool( other.GetBool() ); break;
        case VAR_ARRAY_OF_STRINGS:  SetValueArrayOfStrings( other.GetArrayOfStrings() ); break;
        case VAR_INT:               SetValueInt( other.GetInt() ); break;
        case VAR_STRUCT:            SetValueStruct( other.GetStructMembers() ); break;
        case VAR_ARRAY_OF_STRUCTS:  SetValueArrayOfStructs( other.GetArrayOfStructs() ); break;
        case MAX_VAR_TYPES: ASSERT( false ); break;
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const AString & value )
: m_Name( name )
, m_Type( VAR_STRING )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
, m_StringValue( value )
, m_ArrayValues( 0, false )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, bool value )
: m_Name( name )
, m_Type( VAR_BOOL )
, m_Frozen( false )
, m_BoolValue( value )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, false )
, m_StructMembers( 0, false )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const Array< AString > & values )
: m_Name( name )
, m_Type( VAR_ARRAY_OF_STRINGS )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, true )
, m_StructMembers( 0, false )
, m_ArrayOfStructs( 0, false )
{
    m_ArrayValues = values;
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, int i )
: m_Name( name )
, m_Type( VAR_INT )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( i )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, true )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const Array< const BFFVariable * > & values )
: m_Name( name )
, m_Type( VAR_STRUCT )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, false )
, m_StructMembers( values.GetSize(), true )
, m_ArrayOfStructs( 0, false )
{
    SetValueStruct( values );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name,
                          const Array< const BFFVariable * > & structs,
                          VarType type ) // type for disambiguation
: m_Name( name )
, m_Type( VAR_ARRAY_OF_STRUCTS )
, m_Frozen( false )
, m_BoolValue( false )
, m_IntValue( 0 )
//, m_StringValue() // default construct this
, m_ArrayValues( 0, false )
, m_StructMembers( 0, false )
, m_ArrayOfStructs( structs.GetSize(), true )
{
    // type for disambiguation only - sanity check it's the right type
    ASSERT( type == VAR_ARRAY_OF_STRUCTS ); (void)type;

    SetValueArrayOfStructs( structs );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::~BFFVariable()
{
    // clean up struct members
    for ( BFFVariable ** it = m_StructMembers.Begin();
          it != m_StructMembers.End();
          ++it )
    {
        FDELETE *it;
    }

    // clean up arrays of structs
    for ( BFFVariable ** it = m_ArrayOfStructs.Begin();
          it != m_ArrayOfStructs.End();
          ++it )
    {
        FDELETE *it;
    }
}

// SetValueString
//------------------------------------------------------------------------------
void BFFVariable::SetValueString( const AString & value )
{
    ASSERT( false == m_Frozen );
    m_Type = VAR_STRING;
    m_StringValue = value;
}

// SetValueBool
//------------------------------------------------------------------------------
void BFFVariable::SetValueBool( bool value )
{
    ASSERT( false == m_Frozen );
    m_Type = VAR_BOOL;
    m_BoolValue = value;
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( const Array< AString > & values )
{
    ASSERT( false == m_Frozen );
    m_Type = VAR_ARRAY_OF_STRINGS;
    m_ArrayValues = values;
}

// SetValueInt
//------------------------------------------------------------------------------
void BFFVariable::SetValueInt( int i )
{
    ASSERT( false == m_Frozen );
    m_Type = VAR_INT;
    m_IntValue = i;
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( const Array< const BFFVariable * > & values )
{
    ASSERT( false == m_Frozen );

    // build list of new members, but don't touch old ones yet to gracefully
    // handle self-assignment
    Array< BFFVariable * > newVars( values.GetSize(), false );

    m_Type = VAR_STRUCT;
    for ( const BFFVariable ** it = values.Begin();
          it != values.End();
          ++it )
    {
        const BFFVariable * var = *it;
        BFFVariable * newV = FNEW( BFFVariable( *var ) );
        newVars.Append( newV );
    }

    // free old members
    for ( BFFVariable ** it = m_StructMembers.Begin();
          it != m_StructMembers.End();
          ++it )
    {
        FDELETE *it;
    }

    // swap
    m_StructMembers.Swap( newVars );
}

// SetValueArrayOfStructs
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStructs( const Array< const BFFVariable * > & values )
{
    ASSERT( false == m_Frozen );

    // build list of new members, but don't touch old ones yet to gracefully
    // handle self-assignment
    Array< BFFVariable * > newVars( values.GetSize(), false );

    m_Type = VAR_ARRAY_OF_STRUCTS;
    for ( const BFFVariable ** it = values.Begin();
          it != values.End();
          ++it )
    {
        const BFFVariable * var = *it;
        BFFVariable * newV = FNEW( BFFVariable( *var ) );
        newVars.Append( newV );
    }

    // free old members
    for ( BFFVariable ** it = m_ArrayOfStructs.Begin();
          it != m_ArrayOfStructs.End();
          ++it )
    {
        FDELETE *it;
    }

    m_ArrayOfStructs.Swap( newVars );
}

// GetMemberByName
//------------------------------------------------------------------------------
/*static*/ const BFFVariable ** BFFVariable::GetMemberByName( const AString & name, const Array< const BFFVariable * > & members )
{
    ASSERT( !name.IsEmpty() );

    for ( const BFFVariable ** it = members.Begin(); it != members.End(); ++it )
    {
        if ( (*it)->GetName() == name )
        {
            return it;
        }
    }

    return nullptr;
}

// ConcatVarsRecurse
//------------------------------------------------------------------------------
BFFVariable * BFFVariable::ConcatVarsRecurse( const AString & dstName, const BFFVariable & other, const BFFIterator & operatorIter ) const
{
    const BFFVariable *varDst = this;
    const BFFVariable *varSrc = &other;

    const VarType dstType = m_Type;
    const VarType srcType = other.m_Type;

    // handle supported types

    if ( srcType != dstType )
    {
        // Mismatched - is there a supported conversion?

        // String to ArrayOfStrings
        if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) &&
             ( srcType == BFFVariable::VAR_STRING) )
        {
            uint32_t num = (uint32_t)(1 + other.GetArrayOfStrings().GetSize());
            Array< AString > values(num, false);
            values.Append( varDst->GetArrayOfStrings() );
            values.Append( varSrc->GetString() );

            BFFVariable *result = FNEW(BFFVariable(dstName, values));
            FLOG_INFO("Concatenated <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num);
            return result;
        }

        // Struct to ArrayOfStructs
        if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) &&
             ( srcType == BFFVariable::VAR_STRUCT ) )
        {
            uint32_t num = (uint32_t)( 1 + varDst->GetArrayOfStructs().GetSize() );
            Array< const BFFVariable * > values( num, false );
            values.Append( varDst->GetArrayOfStructs() );
            values.Append( varSrc );

            BFFVariable *result = FNEW( BFFVariable( dstName, values ) );
            FLOG_INFO( "Concatenated <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
            return result;
        }

        // Incompatible types
        Error::Error_1034_OperationNotSupported( operatorIter, // TODO:C we don't have access to the rhsIterator so we use the operator
                                                 varDst ? varDst->GetType() : varSrc->GetType(),
                                                 varSrc->GetType(),
                                                 operatorIter );
        return nullptr;
    }
    else
    {
        // Matching Src and Dst

        if ( srcType == BFFVariable::VAR_STRING )
        {
            AStackString< 2048 > finalValue;
            finalValue = varDst->GetString();
            finalValue += varSrc->GetString();

            BFFVariable *result = FNEW( BFFVariable( dstName, finalValue ) );
            FLOG_INFO( "Concatenated <string> variable '%s' with value '%s'", dstName.Get(), finalValue.Get() );
            return result;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS )
        {
            const unsigned int num = (unsigned int)( varSrc->GetArrayOfStrings().GetSize() + varDst->GetArrayOfStrings().GetSize() );
            Array< AString > values(num, false);
            values.Append( varDst->GetArrayOfStrings() );
            values.Append( varSrc->GetArrayOfStrings() );

            BFFVariable *result = FNEW( BFFVariable( dstName, values ) );
            FLOG_INFO( "Concatenated <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num );
            return result;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS )
        {
            const unsigned int num = (unsigned int)( varSrc->GetArrayOfStructs().GetSize() + varDst->GetArrayOfStructs().GetSize() );
            Array< const BFFVariable * > values( num, false );
            values.Append( varDst->GetArrayOfStructs() );
            values.Append( varSrc->GetArrayOfStructs() );

            BFFVariable *result = FNEW(BFFVariable(dstName, values));
            FLOG_INFO("Concatenated <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num);
            return result;
        }

        if ( srcType == BFFVariable::VAR_INT )
        {
            int newVal( varSrc->GetInt() );
            newVal += varDst->GetInt();

            BFFVariable * result = FNEW( BFFVariable( dstName, newVal ) );
            FLOG_INFO( "Concatenated <int> variable '%s' with value %d", dstName.Get(), newVal );
            return result;
        }

        if ( srcType == BFFVariable::VAR_BOOL )
        {
            // Assume + <=> OR
            bool newVal( varSrc->GetBool() );
            newVal |= varDst->GetBool();

            BFFVariable * result = FNEW( BFFVariable( dstName, newVal ) );
            FLOG_INFO("Concatenated <bool> variable '%s' with value %d", dstName.Get(), newVal);
            return result;
        }

        if ( srcType == BFFVariable::VAR_STRUCT )
        {
            const Array< const BFFVariable * > & srcMembers = varSrc->GetStructMembers();
            const Array< const BFFVariable * > & dstMembers = varDst->GetStructMembers();

            BFFVariable * const result = FNEW( BFFVariable( dstName, BFFVariable::VAR_STRUCT ) );
            result->m_StructMembers.SetCapacity( srcMembers.GetSize() + dstMembers.GetSize() );
            Array< BFFVariable * > & allMembers = result->m_StructMembers;

            // keep original (dst) members where member is only present in original (dst)
            // or concatenate recursively members where the name exists in both
            for ( const BFFVariable ** it = dstMembers.Begin(); it != dstMembers.End(); ++it )
            {
                const BFFVariable ** it2 = GetMemberByName( (*it)->GetName(), srcMembers );

                BFFVariable * newVar;
                if ( it2 )
                {
                    newVar = (*it)->ConcatVarsRecurse( (*it)->GetName(), **it2, operatorIter );
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
                const BFFVariable ** it2 = GetMemberByName( (*it)->GetName(), result->GetStructMembers() );
                if ( nullptr == it2 )
                {
                    BFFVariable *const newVar = FNEW( BFFVariable( **it ) );
                    allMembers.Append( newVar );
                }
            }

            FLOG_INFO( "Concatenated <struct> variable '%s' with %u members", dstName.Get(), (uint32_t)allMembers.GetSize() );
            return result;
        }
    }

    ASSERT( false ); // Should never get here
    return nullptr;
}

//------------------------------------------------------------------------------
