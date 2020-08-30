//
// An simple object with 2 headers included
//
#include "HeaderA.h"
#include "HeaderB.h"

int main( int argc, char ** )
{
    return functionC( functionB( functionA( argc ) ) );
}
