// BFFStackFrame
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFStackFrame.h"
#include "BFFVariable.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"

#include "Core/Mem/Mem.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

//
/*static*/ BFFStackFrame * BFFStackFrame::s_StackHead = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFStackFrame::BFFStackFrame()
{
    // hook into top of stack chain
    m_Next = s_StackHead;
    m_OldHeadToRestore = nullptr;
    m_Depth = GetDepth() + 1;
    s_StackHead = this;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFStackFrame::~BFFStackFrame()
{
    // unhook from top of stack chain
    ASSERT( s_StackHead == this );

    if ( m_OldHeadToRestore )
    {
        // If disconnected for chain, restore previous chain
        ASSERT( m_Next == nullptr );
        s_StackHead = m_OldHeadToRestore;
    }
    else
    {
        // If part of chain, restore parent to head
        ASSERT( m_OldHeadToRestore == nullptr );
        s_StackHead = m_Next;
    }
}

// DisconnectStackChain
//------------------------------------------------------------------------------
void BFFStackFrame::DisconnectStackChain()
{
    // Record parent to restore to head when we fall out of scope
    m_OldHeadToRestore = m_Next;
    m_Next = nullptr;
}

// SetVarString
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarString( const AString & name,
                                                            const BFFToken & token,
                                                            const AString & value,
                                                            BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueString( value );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, value );
}

// SetVarArrayOfStrings
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarArrayOfStrings( const AString & name,
                                                                    const BFFToken & token,
                                                                    const Array<AString> & values,
                                                                    BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueArrayOfStrings( values );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, values );
}

// SetVarBool
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarBool( const AString & name,
                                                          const BFFToken & token,
                                                          bool value,
                                                          BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueBool( value );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, value );
}

// SetVarInt
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarInt( const AString & name,
                                                         const BFFToken & token,
                                                         int value,
                                                         BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueInt( value );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, value );
}

// SetVarStruct
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarStruct( const AString & name,
                                                            const BFFToken & token,
                                                            const BFFVariableScope & members,
                                                            BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueStruct( members );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, members );
}

// SetVarStruct
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarStruct( const AString & name,
                                                            const BFFToken & token,
                                                            BFFVariableScope && members,
                                                            BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueStruct( Move( members ) );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, Move( members ) );
}

// SetVarArrayOfStructs
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVarArrayOfStructs( const AString & name,
                                                                    const BFFToken & token,
                                                                    const Array<BFFVariableScope> & structs,
                                                                    BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->ForceSetValueArrayOfStructs( structs );
        return var;
    }

    // variable not found at this level, so create it
    return &frame->m_Variables.Emplace( name, name, token, structs );
}

// SetVar
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVar( const BFFVariable * var,
                                                      const BFFToken & token,
                                                      BFFStackFrame * frame )
{
    return SetVar( var, token, var->GetName(), frame );
}

// SetVar
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::SetVar( const BFFVariable * srcVar,
                                                      const BFFToken & token,
                                                      const AString & dstName,
                                                      BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    ASSERT( srcVar );

    if ( ( srcVar->GetName() == dstName ) && ( frame->GetVarNoRecurse( dstName ) == srcVar ) )
    {
        // Self-assignment. Nothing to do.
        return const_cast<BFFVariable *>( srcVar );
    }

    BFFVariable srcVarCopy( *srcVar ); // Local copy in case a variable is begin overwritten with one of its sub-variables.

    switch ( srcVarCopy.GetType() )
    {
        case BFFVariable::VAR_ANY: ASSERT( false ); return nullptr; break;
        case BFFVariable::VAR_STRING: return SetVarString( dstName, token, Move( srcVarCopy.m_StringValue ), frame ); break;
        case BFFVariable::VAR_BOOL: return SetVarBool( dstName, token, Move( srcVarCopy.m_BoolValue ), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRINGS: return SetVarArrayOfStrings( dstName, token, Move( srcVarCopy.m_ArrayValues ), frame ); break;
        case BFFVariable::VAR_INT: return SetVarInt( dstName, token, Move( srcVarCopy.m_IntValue ), frame ); break;
        case BFFVariable::VAR_STRUCT: return SetVarStruct( dstName, token, Move( srcVarCopy.m_StructValue ), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRUCTS: return SetVarArrayOfStructs( dstName, token, Move( srcVarCopy.m_StructArrayValues ), frame ); break;
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); return nullptr; break;
    }
}

// ConcatVars
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::ConcatVars( const AString & name,
                                                          const BFFVariable * lhs,
                                                          const BFFVariable * rhs,
                                                          BFFStackFrame * frame,
                                                          const BFFToken * operatorIter )
{
    frame = frame ? frame : s_StackHead;

    ASSERT( frame );
    ASSERT( lhs );
    ASSERT( rhs );

    // Copy lhs to the current stack frame. No-op if already in stack frame with same name.
	BFFVariable * lhsMutable = SetVar( lhs, lhs->GetToken(), name, frame );

    const bool result = lhsMutable->Concat( *rhs, operatorIter );
    if ( !result )
    {
        // Concat would have emitted an error
        return nullptr;
    }
    return lhsMutable;
}

// GetVar
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::GetVar( const char * name, BFFStackFrame * frame )
{
    AStackString strName( name );
    return GetVar( strName, frame );
}

// GetVar
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::GetVar( const AString & name, BFFStackFrame * frame )
{
    // we shouldn't be calling this if there aren't any stack frames
    ASSERT( s_StackHead );

    if ( frame )
    {
        // no recursion, specific frame provided
        return frame->GetVarMutableNoRecurse( name );
    }
    else
    {
        // recurse up the stack
        return s_StackHead->GetVariableRecurse( name );
    }
}

// GetVariableRecurse
//------------------------------------------------------------------------------
const BFFVariable * BFFStackFrame::GetVariableRecurse( const AString & name ) const
{
    // look at this scope level
    if ( const BFFVariable * var = m_Variables.Find( name ) )
    {
        ASSERT( var->GetName() == name );
        return var;
    }

    // look at parent
    if ( m_Next )
    {
        return m_Next->GetVariableRecurse( name );
    }

    // not found
    return nullptr;
}

// GetLocalVar
//------------------------------------------------------------------------------
const BFFVariable * BFFStackFrame::GetLocalVar( const AString & name ) const
{
    // look at this scope level
    return GetVarNoRecurse( name );
}

// GetParentDeclaration
//------------------------------------------------------------------------------
/*static*/ BFFStackFrame * BFFStackFrame::GetParentDeclaration( const char * name, BFFStackFrame * frame, const BFFVariable *& variable )
{
    AStackString strName( name );
    return GetParentDeclaration( strName, frame, variable );
}

// GetParentDeclaration
//------------------------------------------------------------------------------
/*static*/ BFFStackFrame * BFFStackFrame::GetParentDeclaration( const AString & name, BFFStackFrame * frame, const BFFVariable *& variable )
{
    // we shouldn't be calling this if there aren't any stack frames
    ASSERT( s_StackHead );

    variable = nullptr;

    BFFStackFrame * parentFrame = frame ? frame->GetParent() : GetCurrent()->GetParent();

    // look for the scope containing the original variable
    for ( ; parentFrame; parentFrame = parentFrame->GetParent() )
    {
        if ( ( variable = parentFrame->GetLocalVar( name ) ) != nullptr )
        {
            return parentFrame;
        }
    }

    ASSERT( nullptr == variable );
    return nullptr;
}

// GetVarAny
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::GetVarAny( const AString & nameOnly )
{
    ASSERT( nameOnly.BeginsWith( '.' ) == false ); // Should not include . : TODO:C Resolve the inconsistency

    // we shouldn't be calling this if there aren't any stack frames
    ASSERT( s_StackHead );

    // recurse up the stack
    return s_StackHead->GetVariableRecurse( nameOnly, BFFVariable::VAR_ANY );
}

// GetVariableRecurse
//------------------------------------------------------------------------------
const BFFVariable * BFFStackFrame::GetVariableRecurse( const AString & nameOnly,
                                                       BFFVariable::VarType type ) const
{
    ASSERT( nameOnly.BeginsWith( '.' ) == false ); // Should not include . : TODO:C Resolve the inconsistency

    // >:(
    AStackString name( nameOnly.GetLength() + 1 );
    name += '.' ;
    name += nameOnly;

    // look at this scope level
    if ( const BFFVariable * var = m_Variables.Find( name ) )
    {
        //types match?
        if ( ( type == BFFVariable::VAR_ANY ) ||
             ( type == var->GetType() ) )
        {
            // compare names
            ASSERT( var->GetName() == name );
            return var;
        }
    }

    // look at parent
    if ( m_Next )
    {
        return m_Next->GetVariableRecurse( nameOnly, type );
    }

    // not found
    return nullptr;
}

// GetVarNoRecurse
//------------------------------------------------------------------------------
const BFFVariable * BFFStackFrame::GetVarNoRecurse( const AString & name ) const
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames

    // look at this scope level
    if ( const BFFVariable * var = m_Variables.Find( name ) )
    {
        ASSERT( var->GetName() == name );
        return var;
    }

    return nullptr;
}

// GetVarMutableNoRecurse
//------------------------------------------------------------------------------
BFFVariable * BFFStackFrame::GetVarMutableNoRecurse( const AString & name )
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames

    // look at this scope level
    if ( BFFVariable * var = m_Variables.Find( name ) )
    {
        ASSERT( var->GetName() == name );
        return var;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
