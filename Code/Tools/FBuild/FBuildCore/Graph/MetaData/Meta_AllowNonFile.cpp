// Meta_AllowNonFile
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Meta_AllowNonFile.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_BEGIN( Meta_AllowNonFile, IMetaData, MetaNone() )
REFLECT_END( Meta_AllowNonFile )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Meta_AllowNonFile::Meta_AllowNonFile() = default;

// CONSTRUCTOR
//------------------------------------------------------------------------------
Meta_AllowNonFile::Meta_AllowNonFile( const Node::Type limitToType )
    : m_LimitToTypeEnabled( true )
    , m_LimitToType( limitToType )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Meta_AllowNonFile::~Meta_AllowNonFile() = default;

//------------------------------------------------------------------------------
