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

    void DisconnectStackChain();

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
    static void SetVarStruct( const AString & name,
                              Array<BFFVariable *> && members,
                              BFFStackFrame * frame );
    static void SetVarArrayOfStructs( const AString & name,
                                      const Array< const BFFVariable * > & structs,
                                      BFFStackFrame * frame );

    // set from an existing variable
    static void SetVar( const BFFVariable * var, BFFStackFrame * frame );
    static void SetVar( const BFFVariable * srcVar, const AString & dstName, BFFStackFrame * frame );

    // set from two existing variable
    static BFFVariable * ConcatVars( const AString & name,
                                     const BFFVariable * lhs,
                                     const BFFVariable * rhs,
                                     BFFStackFrame * frame,
                                     const BFFToken * operatorIter );

    // get a variable (caller passes complete name indicating type (user vs system))
    static const BFFVariable * GetVar( const char * name, BFFStackFrame * frame = nullptr );
    static const BFFVariable * GetVar( const AString & name, BFFStackFrame * frame = nullptr );

    // get a variable by name, either user or system
    static const BFFVariable * GetVarAny( const AString & nameOnly );

    // get all variables at this stack level only
    const Array< const BFFVariable * > & GetLocalVariables() const { RETURN_CONSTIFIED_BFF_VARIABLE_ARRAY( m_Variables ) }
    Array<BFFVariable *> & GetLocalVariables() { return m_Variables; }

    // get a variable at this stack level only
    const BFFVariable * GetLocalVar( const AString & name ) const;

    static BFFStackFrame * GetCurrent() { return s_StackHead; }
    static uint32_t        GetDepth() { return s_StackHead ? s_StackHead->m_Depth : 1; }

    static BFFStackFrame * GetParentDeclaration( const char * name, BFFStackFrame * frame, const BFFVariable *& variable );
    static BFFStackFrame * GetParentDeclaration( const AString & name, BFFStackFrame * frame, const BFFVariable *& variable );

    BFFStackFrame * GetParent() const { return m_Next; }

    const BFFVariable * GetVariableRecurse( const AString & name ) const;

    const AString & GetLastVariableSeen() const { return m_LastVariableSeen; }
    BFFStackFrame * GetLastVariableSeenFrame() const { return m_LastVariableSeenFrame; }
    void            SetLastVariableSeen( const AString & varName, BFFStackFrame * frame )
    { 
        m_LastVariableSeen = varName;
        m_LastVariableSeenFrame = frame;
    }

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
    BFFStackFrame * m_OldHeadToRestore;
    uint32_t        m_Depth;

    // Track last variable to allow omission of left hand side in operations on the same var
    AString m_LastVariableSeen;
    BFFStackFrame * m_LastVariableSeenFrame = nullptr;

    // the head of the linked list, from deepest to shallowest
    static BFFStackFrame * s_StackHead;
};

//------------------------------------------------------------------------------
