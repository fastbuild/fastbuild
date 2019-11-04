// Move.h
//------------------------------------------------------------------------------
#pragma once

template<class T> struct RemoveReference        { using type = T; };
template<class T> struct RemoveReference<T&>    { using type = T; };
template<class T> struct RemoveReference<T&&>   { using type = T; };
template<class T> using RemoveReferenceT = typename RemoveReference<T>::type;

//template<class T>
//constexpr RemoveReferenceT<T> &&
//Move( T && arg ) noexcept
//{
//    return ( static_cast<RemoveReferenceT<T> &&>( arg ) );
//}

// Macro equivalent to above to avoid function overhead in debug builds
#define Move( x ) static_cast<RemoveReferenceT<decltype(x)> &&>( x )

//------------------------------------------------------------------------------
