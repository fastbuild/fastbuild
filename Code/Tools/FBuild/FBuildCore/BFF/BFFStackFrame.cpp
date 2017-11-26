// BFFStackFrame
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFStackFrame.h"
#include "BFFVariable.h"
#include "Core/Mem/Mem.h"
#include "Core/Strings/AStackString.h"

//
/*static*/ BFFStackFrame * BFFStackFrame::s_StackHead = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFStackFrame::BFFStackFrame()
: m_Variables( 32, true )
{
    // hook into top of stack chain
    m_Next = s_StackHead;
    s_StackHead = this;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFStackFrame::~BFFStackFrame()
{
    // unhook from top of stack chain
    ASSERT( s_StackHead == this );
    s_StackHead = m_Next;

    // free all variables we own
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        FDELETE *i;
    }
}

// SetVarString
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarString( const AString & name,
                                             const AString & value,
                                             BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueString( value );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, value ) );
    frame->m_Variables.Append( v );
}

// SetVarArrayOfStrings
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarArrayOfStrings( const AString & name,
                                                     const Array< AString > & values,
                                                     BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueArrayOfStrings( values );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, values ) );
    frame->m_Variables.Append( v );
}

// SetVarBool
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarBool( const AString & name, bool value, BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueBool( value );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, value ) );
    frame->m_Variables.Append( v );
}

// SetVarInt
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarInt( const AString & name, int value, BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueInt( value );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, value ) );
    frame->m_Variables.Append( v );
}

// SetVarStruct
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarStruct( const AString & name,
                                             const Array< const BFFVariable * > & members,
                                             BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueStruct( members );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, members ) );
    frame->m_Variables.Append( v );
}

// SetVarArrayOfStructs
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVarArrayOfStructs( const AString & name,
                                                     const Array< const BFFVariable * > & structs,
                                                     BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    BFFVariable * var = frame->GetVarMutableNoRecurse( name );
    if ( var )
    {
        var->SetValueArrayOfStructs( structs );
        return;
    }

    // variable not found at this level, so create it
    BFFVariable * v = FNEW( BFFVariable( name, structs, BFFVariable::VAR_ARRAY_OF_STRUCTS ) );
    frame->m_Variables.Append( v );
}


// SetVar
//------------------------------------------------------------------------------
/*static*/ void BFFStackFrame::SetVar( const BFFVariable * var, BFFStackFrame * frame )
{
    frame = frame ? frame : s_StackHead;
    ASSERT( frame );

    ASSERT( var );

    switch ( var->GetType() )
    {
        case BFFVariable::VAR_ANY:              ASSERT( false ); break;
        case BFFVariable::VAR_STRING:           SetVarString( var->GetName(), var->GetString(), frame ); break;
        case BFFVariable::VAR_BOOL:             SetVarBool( var->GetName(), var->GetBool(), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRINGS: SetVarArrayOfStrings( var->GetName(), var->GetArrayOfStrings(), frame ); break;
        case BFFVariable::VAR_INT:              SetVarInt( var->GetName(), var->GetInt(), frame ); break;
        case BFFVariable::VAR_STRUCT:           SetVarStruct( var->GetName(), var->GetStructMembers(), frame ); break;
        case BFFVariable::VAR_ARRAY_OF_STRUCTS: SetVarArrayOfStructs( var->GetName(), var->GetArrayOfStructs(), frame ); break;
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); break;
    }
}

// ConcatVars
//------------------------------------------------------------------------------
BFFVariable * BFFStackFrame::ConcatVars( const AString & name,
                                         const BFFVariable * lhs,
                                         const BFFVariable * rhs,
                                         BFFStackFrame * frame,
                                         const BFFIterator & operatorIter )
{
    frame = frame ? frame : s_StackHead;

    ASSERT( frame );
    ASSERT( lhs );
    ASSERT( rhs );

    BFFVariable *const newVar = lhs->ConcatVarsRecurse( name, *rhs, operatorIter );
    if ( newVar == nullptr )
    {
        return nullptr; // ConcatVarsRecurse will have emitted an error
    }
    frame->CreateOrReplaceVarMutableNoRecurse( newVar );

    return newVar;
}

// GetVar
//------------------------------------------------------------------------------
/*static*/ const BFFVariable * BFFStackFrame::GetVar( const char * name, BFFStackFrame * frame )
{
    AStackString<> strName( name );
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
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        if ( ( *i )->GetName() == name )
        {
            return *i;
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
const BFFVariable * BFFStackFrame::GetLocalVar( const AString & name ) const
{
    // look at this scope level
    return GetVarNoRecurse( name );
}

// GetParentDeclaration
//------------------------------------------------------------------------------
/*static*/ BFFStackFrame * BFFStackFrame::GetParentDeclaration( const char * name, BFFStackFrame * frame, const BFFVariable *& variable )
{
    AStackString<> strName( name );
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
/*static*/ const BFFVariable * BFFStackFrame::GetVarAny( const AString & name )
{
    // we shouldn't be calling this if there aren't any stack frames
    ASSERT( s_StackHead );

    // recurse up the stack
    return s_StackHead->GetVariableRecurse( name, BFFVariable::VAR_ANY );
}

// GetVariableRecurse
//------------------------------------------------------------------------------
const BFFVariable * BFFStackFrame::GetVariableRecurse( const AString & nameOnly,
                                                 BFFVariable::VarType type ) const
{
    // look at this scope level
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        // if name only (minus type prefix ) length matches
        if ( ( *i )->GetName().GetLength() == ( nameOnly.GetLength() + 1 ) )
        {
            //types match?
            if ( ( type == BFFVariable::VAR_ANY ) ||
                 ( type == ( *i )->GetType() ) )
            {
                // compare names
                if ( nameOnly == ( ( *i )->GetName().Get() + 1 ) )
                {
                    return *i;
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
const BFFVariable * BFFStackFrame::GetVarNoRecurse( const AString & name ) const
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames

    // look at this scope level
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        if ( ( *i )->GetName() == name )
        {
            return *i;
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
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        if ( ( *i )->GetName() == name )
        {
            return *i;
        }
    }

    return nullptr;
}

// CreateOrReplaceVarMutableNoRecurse
//------------------------------------------------------------------------------
void BFFStackFrame::CreateOrReplaceVarMutableNoRecurse( BFFVariable *var )
{
    ASSERT( s_StackHead ); // we shouldn't be calling this if there aren't any stack frames
    ASSERT( var );

    // look at this scope level
    Array< BFFVariable * >::Iter i = m_Variables.Begin();
    Array< BFFVariable * >::Iter end = m_Variables.End();
    for( ; i < end ; ++i )
    {
        if ( ( *i )->GetName() == var->GetName() )
        {
            FDELETE *i;
            *i = var;
            return;
        }
    }

    m_Variables.Append( var );
}

//------------------------------------------------------------------------------
