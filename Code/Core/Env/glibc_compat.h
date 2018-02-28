// glibc_compat
//
// Target older versions of glibc for better Linux distro compatibility
//------------------------------------------------------------------------------
#pragma once

// Avoid "Fortify Source" checks
//------------------------------------------------------------------------------
#undef _FORTIFY_SOURCE

// Use older memcpy
//------------------------------------------------------------------------------
__asm__( ".symver memcpy,memcpy@GLIBC_2.2.5" );

//------------------------------------------------------------------------------
