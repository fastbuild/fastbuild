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
    m_Variables.SetCapacity( 32 );

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

    // free all variables we own
    for ( BFFVariable * var : m_Variables )
    {
        FDELETE var;
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
/*static*/ const BFFVariable * BFFStackFrame::SetVarString( const AString & name,
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
    return &frame->m_Variables.EmplaceBack( name, token, value );
}

// SetVarArrayOfStrings
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarArrayOfStrings( const AString & name,
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
    return &frame->m_Variables.EmplaceBack( name, token, values );
}

// SetVarBool
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarBool( const AString & name,
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
    return &frame->m_Variables.EmplaceBack( name, token, value );
}

// SetVarInt
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarInt( const AString & name,
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
    return &frame->m_Variables.EmplaceBack( name, token, value );
}

// SetVarStruct
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarStruct( const AString & name,
                                                            const BFFToken & token,
                                                            const Array<BFFVariable> & members,
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
    return &frame->m_Variables.EmplaceBack( name, token, members );
}

// SetVarStruct
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarStruct( const AString & name,
                                                            const BFFToken & token,
                                                            Array<BFFVariable> && members,
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
    return &frame->m_Variables.EmplaceBack( name, token, Move( members ) );
}

// SetVarArrayOfStructs
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::SetVarArrayOfStructs( const AString & name,
                                                                    const BFFToken & token,
                                                                    const Array<BFFVariable> & structs,
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
    return &frame->m_Variables.EmplaceBack( name, token, structs, BFFVariable::VAR_ARRAY_OF_STRUCTS );
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

    if ( frame->m_Variables.Contains( srcVar ) && srcVar->GetName() == dstName )
    {
        // Self-assignment. Nothing to do.
        return srcVar;
    }

    switch ( srcVar->GetType() )
    {
        case BFFVariable::VAR_ANY: ASSERT( false ); break;
        case BFFVariable::VAR_STRING: return SetVarString( dstName, token, srcVar->GetString(), frame ); break;
        case BFFVariable::VAR_BOOL: return SetVarBool( dstName, token, srcVar->GetBool(), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRINGS: return SetVarArrayOfStrings( dstName, token, srcVar->GetArrayOfStrings(), frame ); break;
        case BFFVariable::VAR_INT: return SetVarInt( dstName, token, srcVar->GetInt(), frame ); break;
        case BFFVariable::VAR_STRUCT: return SetVarStruct( dstName, token, srcVar->GetStructMembers(), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRUCTS: return SetVarArrayOfStructs( dstName, token, srcVar->GetArrayOfStructs(), frame ); break;
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); break;
    }
}

// ConcatVars
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::ConcatVars( const AString & name,
                                                          BFFVariable * lhs,
                                                          const BFFVariable * rhs,
                                                          BFFStackFrame * frame,
                                                          const BFFToken * operatorIter )
{
    frame = frame ? frame : s_StackHead;

    ASSERT( frame );
    ASSERT( lhs );
    ASSERT( rhs );

    // Copy lhs to the current stack frame. No-op if already in stack frame with same name.
    lhs = SetVar( lhs, lhs->GetToken(), name, frame );

    // make doubly sure that it is safe to do a const cast
    ASSERT( m_Variables.Contains( lhs ) );

    const bool result = lhs->Concatenate( rhs, operatorIter );
    if ( !result )
    {
        // Concat would have emitted an error
        return nullptr;
    }
    return lhs;
}

// GetVar
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::GetVar( const char * name, BFFStackFrame * frame )
{
    AStackString strName( name );
    return GetVar( strName, frame );
}

// GetVar
//------------------------------------------------------------------------------
/*static*/ BFFVariable * BFFStackFrame::GetVar( const AString & name, BFFStackFrame * frame )
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
BFFVariable * BFFStackFrame::GetVariableRecurse( const AString & name ) const
{
    // look at this scope level
    for ( BFFVariable & var : m_Variables )
    {
        if ( var->GetName() == name )
        {
            return var;
        }
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
BFFVariable * BFFStackFrame::GetLocalVar( const AString & name ) const
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
/*static*/ BFFVariable * BFFStackFrame::GetVarAny( const AString & nameOnly )
{
    ASSERT( nameOnly.BeginsWith( '.' ) == false ); // Should not include . : TODO:C Resolve the inconsistency

    // we shouldn't be calling this if there aren't any stack frames
    ASSERT( s_StackHead );

    // recurse up the stack
    return s_StackHead->GetVariableRecurse( nameOnly, BFFVariable::VAR_ANY );
}

// GetVariableRecurse
//------------------------------------------------------------------------------
BFFVariable * BFFStackFrame::GetVariableRecurse( const AString & nameOnly,
                                                       BFFVariable::VarType type ) const
{
    ASSERT( nameOnly.BeginsWith( '.' ) == false ); // Should not include . : TODO:C Resolve the inconsistency

    // look at this scope level
    for ( BFFVariable * var : m_Variables )
    {
        // if name only (minus type prefix) length matches
        if ( var->GetName().GetLength() == ( nameOnly.GetLength() + 1 ) )
        {
            //types match?
            if ( ( type == BFFVariable::VAR_ANY ) ||
                 ( type == var->GetType() ) )
            {
                // compare names
                if ( nameOnly == ( var->GetName().Get() + 1 ) )
                {
                    return var;
                }
            }
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
BFFVariable * BFFStackFrame::GetVarNoRecurse( const AString & name ) const
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames

    // look at this scope level
    for ( BFFVariable * var : m_Variables )
    {
        if ( var->GetName() == name )
        {
            return var;
        }
    }

    return nullptr;
}

// GetVarMutableNoRecurse
//------------------------------------------------------------------------------
BFFVariable * BFFStackFrame::GetVarMutableNoRecurse( const AString & name )
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames

    // look at this scope level
    for ( BFFVariable * var : m_Variables )
    {
        if ( var->GetName() == name )
        {
            return var;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
