// libc_start_main
//
// A workaround for changes in glibc 2.34 that made programs linked with it incompatible with older libc.so.
//
// In glibc 2.34 the way in which global initialization and destruction functions are called was changed:
// https://sourceware.org/git/?p=glibc.git;a=commitdiff;h=035c012e32c11e84d64905efaf55e74f704d3668
//
// Previously each executable contained functions __libc_csu_init and __libc_csu_fini that iterated
// through all global initialization and destruction functions and the program entry point (_start)
// passed their addresses to the main startup code (__libc_start_main) implemented in libc.so.
//
// In glibc 2.34 __libc_csu_init and __libc_csu_fini are removed, _start passes null pointers to __libc_start_main
// and __libc_start_main finds and executes global initialization and destruction functions by itself.
// As this new behavior of _start is incompatible with __libc_start_main present in old libc.so,
// __libc_start_main got a new symbol version and now programs built with glibc 2.34 link to __libc_start_main@@GLIBC_2.34.
//
// To be able to continue to use any glibc to build executables that will run on older systems we undo all of that:
// 1. We implement our own __libc_csu_init and __libc_csu_fini when building with glibc >= 2.34.
// 2. We hook calls __libc_start_main by using --wrap= linker option.
// 3. In our hook for __libc_start_main we call the real old __libc_start_main.
// 4. If we are building with glibc >= 2.34 we pass addresses of our __libc_csu_* implementations to __libc_start_main.
//------------------------------------------------------------------------------
#if defined( __LINUX__ )

// Includes
//------------------------------------------------------------------------------
#include <errno.h> // for __GLIBC__ and __GLIBC_MINOR__

#if defined( __GLIBC__ )
    #if ( __GLIBC__ * 1000 + __GLIBC_MINOR__ ) >= 2034

        extern void (*__init_array_start[])( int, char **, char ** );
        extern void (*__init_array_end[])( int, char **, char ** );
        extern void (*__fini_array_start[])( void );
        extern void (*__fini_array_end[])( void );

        extern "C" void _init();
        extern "C" void _fini();

        // We are using same names as in glibc for those functions (and not making them static).
        // That way we will get a "multiple definition" linker error if they will be somehow pulled from glibc.
        extern "C" int __libc_csu_init( int argc, char ** argv, char ** envp )
        {
            _init();
            for ( auto p = __init_array_start; p != __init_array_end; )
            {
                (*p++)( argc, argv, envp );
            }
            return 0;
        }
        extern "C" void __libc_csu_fini()
        {
            for ( auto p = __fini_array_end; p != __fini_array_start; )
            {
                (*--p)();
            }
            _fini();
        }

    #endif

    extern "C" int __libc_start_main( int (*main)( int, char **, char ** ),
                                      int argc, char ** argv,
                                      int (*init)( int, char **, char ** ),
                                      void (*fini)( void ),
                                      void (*rtld_fini)( void ),
                                      void * stack_end );

    __asm__( ".symver __libc_start_main,__libc_start_main@GLIBC_2.2.5" );

    extern "C" int __wrap___libc_start_main( int (*main)( int, char **, char ** ),
                                             int argc, char ** argv,
                                             int (*init)( int, char **, char ** ),
                                             void (*fini)( void ),
                                             void (*rtld_fini)( void ),
                                             void * stack_end )
    {
        #if ( __GLIBC__ * 1000 + __GLIBC_MINOR__ ) >= 2034
            init = __libc_csu_init;
            fini = __libc_csu_fini;
        #endif
        return __libc_start_main( main, argc, argv, init, fini, rtld_fini, stack_end );
    }
#endif

//------------------------------------------------------------------------------
#endif
