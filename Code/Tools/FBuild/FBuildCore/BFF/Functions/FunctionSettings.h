// FunctionSettings - Manage global settings
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONSETTINGS_H
#define FBUILD_FUNCTIONS_FUNCTIONSETTINGS_H

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
	explicit		FunctionSettings();
	inline virtual ~FunctionSettings() {}

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
#endif // FBUILD_FUNCTIONS_FUNCTIONSETTINGS_H
