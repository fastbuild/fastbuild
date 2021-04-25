// BFFBooleanExpParser
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class BFFTokenRange;
class Function;

// BFFBooleanExpParser
//------------------------------------------------------------------------------
class BFFBooleanExpParser
{
public:
    static bool Parse( const Function * function,
                       const BFFTokenRange & range,
                       bool & expResult );
};

//------------------------------------------------------------------------------
