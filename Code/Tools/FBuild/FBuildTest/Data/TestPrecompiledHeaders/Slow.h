#pragma once

// A deliberately slow thing to compile, so we can easily see when the
// precompilation is working.
//
// Adapted from: https://randomascii.wordpress.com/2014/03/10/making-compiles-slow/
//
template <int TreePos, int N>
struct FibSlow_t
{
    enum { value = FibSlow_t<TreePos, N - 1>::value +
           FibSlow_t<TreePos + (1 << N), N - 2>::value, };
};

// Explicitly specialized for N==2
template <int T> struct FibSlow_t<T, 2> { enum { value = 1 }; };

// Explicitly specialized for N==1
template <int T> struct FibSlow_t<T, 1> { enum { value = 1 }; };

inline int SlowFunc2() { return FibSlow_t<0,18>::value; }
