//
// LightCache does not support #import directives.
// But it should detect this case and safely fall back to regular cache.
//
//------------------------------------------------------------------------------
#if 0 // We don't want to actually compile an #import directive. Compilation can fail for multiple unrelated reasons and we don't want false positives in tests.
    #import <C:\Windows\system32\stdole32.tlb>
#endif
