
#define MC_ASSERT(X) (X)

int main()
{
    // Clang will suppress the "-Wtautological-compare" because it comes from within a macro
    // However, preprocessing will strip the macro, causing inconsistent behaviour.
    // Using "-rewrite-includes" avoids macro expansion during preprocessing and keeps
    // the behaviour consistent

    unsigned int i = 0;
    bool result = MC_ASSERT(i >= 0);
    return result;
}
