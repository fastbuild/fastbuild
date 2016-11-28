// BFFStackFrame - manages variable scope during BFF parsing
//------------------------------------------------------------------------------
#pragma once

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
                              BFFStackFrame * frame );
    static void SetVarArrayOfStrings( const AString & name,
                                      const Array< AString > & values,
                                      BFFStackFrame * frame );
    static void SetVarBool( const AString & name,
                            bool value,
                            BFFStackFrame * frame );
    static void SetVarInt( const AString & name,
                           int value,
                           BFFStackFrame * frame );
    static void SetVarStruct( const AString & name,
                              const Array< const BFFVariable * > & members,
                              BFFStackFrame * frame );
    static void SetVarArrayOfStructs( const AString & name,
                                      const Array< const BFFVariable * > & structs,
                                      BFFStackFrame * frame );

    // set from an existing variable
    static void SetVar( const BFFVariable * var, BFFStackFrame * frame );

    // set from two existing variable
    static BFFVariable * ConcatVars( const AString & name,
                                     const BFFVariable * lhs,
                                     const BFFVariable * rhs,
                                     BFFStackFrame * frame );

    // get a variable (caller passes complete name indicating type (user vs system))
    static const BFFVariable * GetVar( const char * name, BFFStackFrame * frame = nullptr );
    static const BFFVariable * GetVar( const AString & name, BFFStackFrame * frame = nullptr );

    // get a variable by name, either user or system
    static const BFFVariable * GetVarAny( const AString & name );

    // get all variables at this stack level only
    const Array< const BFFVariable * > & GetLocalVariables() const { RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( m_Variables ); }

    // get a variable at this stack level only
    const BFFVariable * GetLocalVar( const AString & name ) const;

    static BFFStackFrame * GetCurrent() { return s_StackHead; }

    static BFFStackFrame * GetParentDeclaration( const char * name, BFFStackFrame * frame, const BFFVariable *& variable );
    static BFFStackFrame * GetParentDeclaration( const AString & name, BFFStackFrame * frame, const BFFVariable *& variable );

    BFFStackFrame * GetParent() const { return m_Next; }

    const BFFVariable * GetVariableRecurse( const AString & name ) const;

private:
    const BFFVariable * GetVariableRecurse( const AString & nameOnly,
                                      BFFVariable::VarType type ) const;

    const BFFVariable * GetVarNoRecurse( const AString & name ) const;
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
