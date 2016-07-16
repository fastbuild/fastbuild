// FunctionUsing
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionUsing.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionUsing::FunctionUsing()
: Function( "Using" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::NeedsHeader() const
{
	return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::NeedsBody() const
{
	return false;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::ParseFunction( NodeGraph & /*nodeGraph*/,
											   const BFFIterator & functionNameStart,
											   const BFFIterator * functionBodyStartToken, 
											   const BFFIterator * functionBodyStopToken,
											   const BFFIterator * functionHeaderStartToken,
											   const BFFIterator * functionHeaderStopToken ) const
{
	(void)functionNameStart;
	(void)functionBodyStartToken;
	(void)functionBodyStopToken;

	if ( functionHeaderStartToken && functionHeaderStopToken && 
		 ( functionHeaderStartToken->GetDistTo( *functionHeaderStopToken ) >= 1 ) )
	{
		BFFIterator start( *functionHeaderStartToken );
		ASSERT( *start == BFFParser::BFF_FUNCTION_ARGS_OPEN );
		start++;
		start.SkipWhiteSpace();

		// a variable name?
		const char c = *start;
		if ( c != BFFParser::BFF_DECLARE_VAR_INTERNAL &&
		 	 c != BFFParser::BFF_DECLARE_VAR_PARENT )
		{
			Error::Error_1007_ExpectedVariable( start, this ); 
			return false;
		}

		// we want 1 frame above this function
		BFFStackFrame * frame = BFFStackFrame::GetCurrent()->GetParent();
		ASSERT( frame );

		// find variable name
		BFFIterator stop( start );
		AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
		bool parentScope = false;
		if ( BFFParser::ParseVariableName( stop, varName, parentScope ) == false )
		{
			return false;
		}

		ASSERT( stop.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated

		// find variable
		const BFFVariable * v = nullptr;
		BFFStackFrame * const varFrame = ( parentScope )
			? BFFStackFrame::GetParentDeclaration( varName, frame, v )
			: nullptr;

		if ( false == parentScope )
		{
			v = BFFStackFrame::GetVar( varName, nullptr );
		}

		if ( ( parentScope && ( nullptr == varFrame ) ) || ( nullptr == v ) )
	    {
	    	Error::Error_1009_UnknownVariable( start, this );
	    	return false;
	    }

		if ( v->IsStruct() == false )
		{
			Error::Error_1008_VariableOfWrongType( start, this,
												   BFFVariable::VAR_STRUCT,
												   v->GetType() );
			return false;
		}

		const Array< const BFFVariable * > & members = v->GetStructMembers();
		for ( const BFFVariable ** it = members.Begin();
			  it != members.End();
			  ++it )
		{
			const BFFVariable * member = ( *it );
			BFFStackFrame::SetVar( member, frame );
		}
	}

	return true;
}

//------------------------------------------------------------------------------
