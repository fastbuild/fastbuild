// BFFVariable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFVariable.h"

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

// CONSTRUCTOR (copy)
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const BFFVariable & other )
: m_Name( other.m_Name )
, m_Type( other.m_Type )
//, m_StringValue() // default construct this
, m_BoolValue( false )
, m_ArrayValues( 0, true )
, m_IntValue( 0 )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, true )
{
	switch( m_Type )
	{
		case VAR_ANY:				ASSERT( false ); break;
		case VAR_STRING:			SetValueString( other.GetString() ); break;
		case VAR_BOOL:				SetValueBool( other.GetBool() ); break;
		case VAR_ARRAY_OF_STRINGS:	SetValueArrayOfStrings( other.GetArrayOfStrings() ); break;
		case VAR_INT:				SetValueInt( other.GetInt() ); break;
		case VAR_STRUCT:			SetValueStruct( other.GetStructMembers() ); break;
		case VAR_ARRAY_OF_STRUCTS:	SetValueArrayOfStructs( other.GetArrayOfStructs() ); break;
		case MAX_VAR_TYPES:	ASSERT( false ); break;
	}
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const AString & value )
: m_Name( name )
, m_Type( VAR_STRING )
, m_StringValue( value )
, m_BoolValue( false )
, m_ArrayValues( 0, false )
, m_IntValue( 0 )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, bool value )
: m_Name( name )
, m_Type( VAR_BOOL )
//, m_StringValue() // default construct this
, m_BoolValue( value )
, m_ArrayValues( 0, false )
, m_IntValue( 0 )
, m_StructMembers( 0, false )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const Array< AString > & values )
: m_Name( name )
, m_Type( VAR_ARRAY_OF_STRINGS )
//, m_StringValue() // default construct this
, m_BoolValue( false )
, m_ArrayValues( 0, true )
, m_IntValue( 0 )
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
//, m_StringValue() // default construct this
, m_BoolValue( false )
, m_ArrayValues( 0, true )
, m_IntValue( i )
, m_StructMembers( 0, true )
, m_ArrayOfStructs( 0, false )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariable::BFFVariable( const AString & name, const Array< const BFFVariable * > & values )
: m_Name( name )
, m_Type( VAR_STRUCT )
//, m_StringValue() // default construct this
, m_BoolValue( false )
, m_ArrayValues( 0, false )
, m_IntValue( 0 )
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
//, m_StringValue() // default construct this
, m_BoolValue( false )
, m_ArrayValues( 0, false )
, m_IntValue( 0 )
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
	m_Type = VAR_STRING;
	m_StringValue = value;
}

// SetValueBool
//------------------------------------------------------------------------------
void BFFVariable::SetValueBool( bool value )
{
	m_Type = VAR_BOOL;
	m_BoolValue = value;
}

// SetValueArrayOfStrings
//------------------------------------------------------------------------------
void BFFVariable::SetValueArrayOfStrings( const Array< AString > & values )
{
	m_Type = VAR_ARRAY_OF_STRINGS;
	m_ArrayValues = values;
}

// SetValueInt
//------------------------------------------------------------------------------
void BFFVariable::SetValueInt( int i )
{
	m_Type = VAR_INT;
	m_IntValue = i;
}

// SetValueStruct
//------------------------------------------------------------------------------
void BFFVariable::SetValueStruct( const Array< const BFFVariable * > & values )
{
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

//------------------------------------------------------------------------------
