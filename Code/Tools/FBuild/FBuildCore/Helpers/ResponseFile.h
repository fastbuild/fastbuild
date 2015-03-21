// ResponseFile
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_HELPERS_RESPONSEFILE_H
#define FBUILD_HELPERS_RESPONSEFILE_H

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Includes
//------------------------------------------------------------------------------
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// ResponseFile
//------------------------------------------------------------------------------
class ResponseFile
{
public:
	explicit ResponseFile();
	~ResponseFile();

	bool Create( const AString & contents );
	const AString & GetResponseFilePath() const { return m_ResponseFilePath; }

	void SetEscapeSlashes() { m_EscapeSlashes = true; }
private:
	bool CreateInternal( const AString & contents );

	FileStream m_File;
	AStackString<> m_ResponseFilePath;
	bool m_EscapeSlashes;
};

//------------------------------------------------------------------------------
#endif // FBUILD_HELPERS_RESPONSEFILE_H
