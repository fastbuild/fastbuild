// Args
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AStackString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Args
//------------------------------------------------------------------------------
class Args
{
public:
    Args();
    ~Args();

    // Construct args
    void operator += ( const char * argPart );
    void operator += ( const AString & argPart );
    void operator += ( char argPart );
    void Append( const char * begin, size_t count );
    void AddDelimiter();

    void Clear();

    // Set Response File options
    void SetEscapeSlashesInResponseFile() { ASSERT( !m_Finalized ); m_ResponseFile.SetEscapeSlashes(); }

    // Do final fixups and create response file if needed/supported
    bool Finalize( const AString & exe, const AString & nodeNameForError, bool canUseResponseFile );

    // After finalization, access args
    const AString& GetRawArgs() const   { return m_Args; }
    const AString& GetFinalArgs() const { ASSERT( m_Finalized ); return m_ResponseFileArgs.IsEmpty() ? m_Args : m_ResponseFileArgs; }

    // helper functions
    static void StripQuotes( const char * start, const char * end, AString & out );

protected:
    AStackString< 4096 >    m_Args;
    AString                 m_ResponseFileArgs;
    Array< uint32_t >       m_DelimiterIndices;
    ResponseFile            m_ResponseFile;
    #if defined( ASSERTS_ENABLED )
        bool                m_Finalized;
    #endif
};

//------------------------------------------------------------------------------
