// FunctionSettings - Manage global settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// FunctionSettings
//------------------------------------------------------------------------------
class FunctionSettings : public Function
{
public:
    explicit        FunctionSettings();
    inline virtual ~FunctionSettings() = default;

    static inline void SetCachePath( const AString & cachePath ) { s_CachePath = cachePath; }
    static inline const AString & GetCachePath() { return s_CachePath; }

protected:
    virtual bool IsUnique() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;

private:
    void ProcessEnvironment( const Array< AString > & envStrings ) const;

    static AString s_CachePath;
};

//------------------------------------------------------------------------------
