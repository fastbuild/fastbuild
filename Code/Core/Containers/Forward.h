// Forward.h
//------------------------------------------------------------------------------
#pragma once

template < class T, class U > struct ForwardType                { using type = T &&; };
template < class T, class U > struct ForwardType< T &, U & >    { using type = T &;  };
template < class T, class U > struct ForwardType< T &, U >      { static_assert( ( sizeof( T ), false ), "can not forward rvalue as lvalue" ); };

// Macro equivalent to std::forward to avoid function overhead in debug builds
#define Forward( T, x ) static_cast< typename ForwardType< T, decltype( x ) >::type>( x )

//------------------------------------------------------------------------------
