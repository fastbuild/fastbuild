// MSVCStaticAnalysis
//------------------------------------------------------------------------------
//
// Wrapper for MSVC SAL annotations to make them a little easier to use
//
//------------------------------------------------------------------------------
#pragma once

#if defined( __WINDOWS__ ) && defined( ANALYZE )
    #include <sal.h>

    // printf style function
    #define MSVC_SAL_PRINTF _In_z_ _Printf_format_string_
    #define MSVC_SAL_SCANF _Scanf_s_format_string_
#else
    #define MSVC_SAL_PRINTF
    #define MSVC_SAL_SCANF
#endif

//------------------------------------------------------------------------------
