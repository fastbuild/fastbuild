// DTLTOData - Parsed LLVM DTLTO distribution description
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// DTLTOData
//------------------------------------------------------------------------------
struct DTLTOData
{
    struct Job
    {
        Array<AString> m_Args;
        Array<AString> m_Inputs;
        Array<AString> m_Outputs;
    };

    AString m_LinkerOutput;
    AString m_Compiler;
    Array<AString> m_CommonArgs;
    Array<AString> m_CommonInputs;
    Array<Job> m_Jobs;
};

//------------------------------------------------------------------------------
