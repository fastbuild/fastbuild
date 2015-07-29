// BFFStackFrame - manages variable scope during BFF parsing
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_STACKFRAME_H
#define FBUILD_STACKFRAME_H

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// BFFStackFrame
//------------------------------------------------------------------------------
class BFFStackFrame
{
public:
	explicit BFFStackFrame();
	~BFFStackFrame();

	// set the value of a variable
	static void SetVarString( const AString & name,
							  const AString & value,
							  BFFStackFrame * frame = nullptr );
	static void SetVarArrayOfStrings( const AString & name,
									  const Array< AString > & values,
									  BFFStackFrame * frame = nullptr );
	static void SetVarBool( const AString & name,
							bool value,
							BFFStackFrame * frame = nullptr );
	static void SetVarInt( const AString & name,
						   int value,
						   BFFStackFrame * frame = nullptr );
	static void SetVarStruct( const AString & name,
							  const Array< const BFFVariable * > & members,
							  BFFStackFrame * frame = nullptr );
	static void SetVarArrayOfStructs( const AString & name,
									  const Array< const BFFVariable * > & structs,
									  BFFStackFrame * frame = nullptr );

	// set from an existing variable
	static void SetVar( const BFFVariable * var, BFFStackFrame * frame = nullptr );

    // set from two existing variable
    static BFFVariable * ConcatVars( const AString & name,
                                     const BFFVariable * lhs,
                                     const BFFVariable * rhs,
                                     BFFStackFrame * frame = nullptr );

	// get a variable (caller passes complete name indicating type (user vs system))
	static const BFFVariable * GetVar( const char * name );
	static const BFFVariable * GetVar( const AString & name );

	// get a variable by name, either user or system
	static const BFFVariable * GetVarAny( const AString & name );

	// get all variables ar this stack level only
	const Array< const BFFVariable * > & GetLocalVariables() const { RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( m_Variables ); }

	static BFFStackFrame * GetCurrent() { return s_StackHead; }

	BFFStackFrame * GetParent() const { return m_Next; }

private:
	// internal helper
	const BFFVariable * GetVariableRecurse( const AString & name ) const;

	const BFFVariable * GetVariableRecurse( const AString & nameOnly, 
									  BFFVariable::VarType type ) const;
	BFFVariable * GetVarMutableNoRecurse( const AString & name );

    void CreateOrReplaceVarMutableNoRecurse( BFFVariable * var );

	// variables at current scope
	Array< BFFVariable * > m_Variables;

	// pointer to parent scope
	BFFStackFrame * m_Next;

	// the head of the linked list, from deepest to shallowest
	static BFFStackFrame * s_StackHead; 
};

//------------------------------------------------------------------------------
#endif // FBUILD_STACKFRAME_H
 