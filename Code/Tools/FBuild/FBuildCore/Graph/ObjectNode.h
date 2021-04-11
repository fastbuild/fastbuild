// ObjectNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Assert.h"
#include "Core/Process/Process.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class CompilerDriverBase;
class ConstMemoryStream;
class Function;
class NodeGraph;
class NodeProxy;
class ObjectNode;
enum class ArgsResponseFileMode : uint32_t;

// ObjectNode
//------------------------------------------------------------------------------
class ObjectNode : public FileNode
{
    REFLECT_NODE_DECLARE( ObjectNode )
public:
    ObjectNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    // simplified remote constructor
    explicit ObjectNode( const AString & objectName,
                         NodeProxy * srcFile,
                         const AString & compilerOptions,
                         uint32_t flags );
    virtual ~ObjectNode() override;

    static inline Node::Type GetTypeS() { return Node::OBJECT_NODE; }

    class CompilerFlags
    {
    public:
        bool IsCacheable() const                    { return ( ( m_Flags & FLAG_CAN_BE_CACHED ) != 0 ); }
        bool IsDistributable() const                { return ( ( m_Flags & FLAG_CAN_BE_DISTRIBUTED ) != 0 ); }
        bool IsUsingPCH() const                     { return ( ( m_Flags & FLAG_USING_PCH ) != 0 ); }
        bool IsGCC() const                          { return ( ( m_Flags & FLAG_GCC ) != 0 ); }
        bool IsMSVC() const                         { return ( ( m_Flags & FLAG_MSVC ) != 0 ); }
        bool IsCreatingPCH() const                  { return ( ( m_Flags & FLAG_CREATING_PCH ) != 0 ); }
        bool IsSNC() const                          { return ( ( m_Flags & FLAG_SNC ) != 0 ); }
        bool IsUsingCLR() const                     { return ( ( m_Flags & FLAG_USING_CLR ) != 0 ); }
        bool IsClang() const                        { return ( ( m_Flags & FLAG_CLANG ) != 0 ); }
        bool IsUnity() const                        { return ( ( m_Flags & FLAG_UNITY ) != 0 ); }
        bool IsIsolatedFromUnity() const            { return ( ( m_Flags & FLAG_ISOLATED_FROM_UNITY ) != 0 ); }
        bool IsUsingPDB() const                     { return ( ( m_Flags & FLAG_USING_PDB ) != 0 ); }
        bool IsCodeWarriorWii() const               { return ( ( m_Flags & CODEWARRIOR_WII ) != 0 ); }
        bool IsGreenHillsWiiU() const               { return ( ( m_Flags & GREENHILLS_WIIU ) != 0 ); }
        bool IsCUDANVCC() const                     { return ( ( m_Flags & FLAG_CUDA_NVCC ) != 0 ); }
        bool IsIncludesInStdErr() const             { return ( ( m_Flags & FLAG_INCLUDES_IN_STDERR ) != 0 ); }
        bool IsQtRCC() const                        { return ( ( m_Flags & FLAG_QT_RCC ) != 0 ); }
        bool IsWarningsAsErrorsMSVC() const         { return ( ( m_Flags & FLAG_QT_RCC ) != 0 ); }
        bool IsVBCC() const                         { return ( ( m_Flags & FLAG_VBCC ) != 0 ); }
        bool IsUsingStaticAnalysisMSVC() const      { return ( ( m_Flags & FLAG_STATIC_ANALYSIS_MSVC ) != 0 ); }
        bool IsOrbisWavePSSLC() const               { return ( ( m_Flags & FLAG_ORBIS_WAVE_PSSLC ) != 0 ); }
        bool IsDiagnosticsColorAuto() const         { return ( ( m_Flags & FLAG_DIAGNOSTICS_COLOR_AUTO ) != 0 ); }
        bool IsWarningsAsErrorsClangGCC() const     { return ( ( m_Flags & FLAG_WARNINGS_AS_ERRORS_CLANGGCC ) != 0 ); }
        bool IsClangCl() const                      { return ( ( m_Flags & FLAG_CLANG_CL ) != 0 ); }

        enum Flag : uint32_t
        {
            FLAG_CAN_BE_CACHED                  = 0x01,
            FLAG_CAN_BE_DISTRIBUTED             = 0x02,
            FLAG_USING_PCH                      = 0x04,
            FLAG_GCC                            = 0x10,
            FLAG_MSVC                           = 0x20,
            FLAG_CREATING_PCH                   = 0x40,
            FLAG_SNC                            = 0x80,
            FLAG_USING_CLR                      = 0x100,
            FLAG_CLANG                          = 0x200,
            FLAG_UNITY                          = 0x400,
            FLAG_ISOLATED_FROM_UNITY            = 0x800,
            FLAG_USING_PDB                      = 0x1000,
            CODEWARRIOR_WII                     = 0x2000,
            GREENHILLS_WIIU                     = 0x4000,
            FLAG_CUDA_NVCC                      = 0x10000,
            FLAG_INCLUDES_IN_STDERR             = 0x20000,
            FLAG_QT_RCC                         = 0x40000,
            FLAG_WARNINGS_AS_ERRORS_MSVC        = 0x80000,
            FLAG_VBCC                           = 0x100000,
            FLAG_STATIC_ANALYSIS_MSVC           = 0x200000,
            FLAG_ORBIS_WAVE_PSSLC               = 0x400000,
            FLAG_DIAGNOSTICS_COLOR_AUTO         = 0x800000,
            FLAG_WARNINGS_AS_ERRORS_CLANGGCC    = 0x1000000,
            FLAG_CLANG_CL                       = 0x2000000,
        };

        void Set( Flag flag )       { m_Flags |= flag; }
        void Clear( Flag flag )     { m_Flags &= ( ~flag ); }

        uint32_t m_Flags = 0;
    };
    const CompilerFlags& GetCompilerFlags() const { return m_CompilerFlags; }

    static CompilerFlags DetermineFlags( const CompilerNode * compilerNode,
                                         const AString & args,
                                         bool creatingPCH,
                                         bool usingPCH );
    static bool IsCompilerArg_MSVC( const AString & token, const char * arg );
    static bool IsStartOfCompilerArg_MSVC( const AString & token, const char * arg );

    bool IsCacheable() const                { return m_CompilerFlags.IsCacheable(); }
    bool IsDistributable() const            { return m_CompilerFlags.IsDistributable(); }
    bool IsUsingPCH() const                 { return m_CompilerFlags.IsUsingPCH(); }
    bool IsCreatingPCH() const              { return m_CompilerFlags.IsCreatingPCH(); }
    bool IsClang() const                    { return m_CompilerFlags.IsClang(); }
    bool IsUnity() const                    { return m_CompilerFlags.IsUnity(); }
    bool IsIsolatedFromUnity() const        { return m_CompilerFlags.IsIsolatedFromUnity(); }
    bool IsGCC() const                      { return m_CompilerFlags.IsGCC(); }
    bool IsMSVC() const                     { return m_CompilerFlags.IsMSVC(); }
    bool IsSNC() const                      { return m_CompilerFlags.IsSNC(); }
    bool IsUsingCLR() const                 { return m_CompilerFlags.IsUsingCLR(); }
    bool IsClangCl() const                  { return m_CompilerFlags.IsClangCl(); }
    bool IsUsingPDB() const                 { return m_CompilerFlags.IsUsingPDB(); }
    bool IsCodeWarriorWii() const           { return m_CompilerFlags.IsCodeWarriorWii(); }
    bool IsGreenHillsWiiU() const           { return m_CompilerFlags.IsGreenHillsWiiU(); }
    bool IsCUDANVCC() const                 { return m_CompilerFlags.IsCUDANVCC(); }
    bool IsIncludesInStdErr() const         { return m_CompilerFlags.IsIncludesInStdErr(); }
    bool IsQtRCC() const                    { return m_CompilerFlags.IsQtRCC(); }
    bool IsWarningsAsErrorsMSVC() const     { return m_CompilerFlags.IsWarningsAsErrorsMSVC(); }
    bool IsVBCC() const                     { return m_CompilerFlags.IsVBCC(); }
    bool IsUsingStaticAnalysisMSVC() const  { return m_CompilerFlags.IsUsingStaticAnalysisMSVC(); }
    bool IsOrbisWavePSSLC() const           { return m_CompilerFlags.IsOrbisWavePSSLC(); }
    bool IsWarningsAsErrorsClangGCC() const { return m_CompilerFlags.IsWarningsAsErrorsClangGCC(); }

    virtual void SaveRemote( IOStream & stream ) const override;
    static Node * LoadRemote( IOStream & stream );

    CompilerNode * GetCompiler() const;
    inline Node * GetSourceFile() const { return m_StaticDependencies[ 1 ].GetNode(); }
    CompilerNode * GetDedicatedPreprocessor() const;
    #if defined( __WINDOWS__ )
        inline Node * GetPrecompiledHeaderCPPFile() const { ASSERT( m_CompilerFlags.IsCreatingPCH() ); return m_StaticDependencies[ 1 ].GetNode(); }
    #endif
    ObjectNode * GetPrecompiledHeader() const;

    void GetPDBName( AString & pdbName ) const;
    void GetNativeAnalysisXMLPath( AString& outXMLFileName ) const;

    const char * GetObjExtension() const;

    const AString & GetPCHObjectName() const { return m_PCHObjectFileName; }
    const AString & GetOwnerObjectList() const { return m_OwnerObjectList; }

    void ExpandCompilerForceUsing( Args & fullArgs, const AString & pre, const AString & post ) const;

private:
    virtual BuildResult DoBuild( Job * job ) override;
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob ) override;
    virtual bool Finalize( NodeGraph & nodeGraph ) override;

    virtual void Migrate( const Node & oldNode ) override;

    BuildResult DoBuildMSCL_NoCache( Job * job, bool useDeoptimization );
    BuildResult DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache, bool useSimpleDist );
    BuildResult DoBuildWithPreProcessor2( Job * job,
                                          bool useDeoptimization,
                                          bool stealingRemoteJob,
                                          bool racingRemoteJob,
                                          bool isFollowingLightCacheMiss );
    BuildResult DoBuild_QtRCC( Job * job );
    BuildResult DoBuildOther( Job * job, bool useDeoptimization );

    bool ProcessIncludesMSCL( const char * output, uint32_t outputSize );
    bool ProcessIncludesWithPreProcessor( Job * job );

    const AString & GetCacheName( Job * job ) const;
    bool RetrieveFromCache( Job * job );
    void WriteToCache( Job * job );
    void GetExtraCacheFilePaths( const Job * job, Array< AString > & outFileNames ) const;

    void EmitCompilationMessage( const Args & fullArgs, bool useDeoptimization, bool stealingRemoteJob = false, bool racingRemoteJob = false, bool useDedicatedPreprocessor = false, bool isRemote = false ) const;

    enum Pass
    {
        PASS_PREPROCESSOR_ONLY,
        PASS_COMPILE_PREPROCESSED,
        PASS_COMPILE,
        PASS_PREP_FOR_SIMPLE_DISTRIBUTION,
    };
    bool BuildArgs( const Job * job, Args & fullArgs, Pass pass, bool useDeoptimization, bool useShowIncludes, bool useSourceMapping, bool finalize, const AString & overrideSrcFile = AString::GetEmpty() ) const;

    bool BuildPreprocessedOutput( const Args & fullArgs, Job * job, bool useDeoptimization ) const;
    bool LoadStaticSourceFileForDistribution( const Args & fullArgs, Job * job, bool useDeoptimization ) const;
    void TransferPreprocessedData( const char * data, size_t dataSize, Job * job ) const;
    bool WriteTmpFile( Job * job, AString & tmpDirectory, AString & tmpFileName ) const;
    bool BuildFinalOutput( Job * job, const Args & fullArgs ) const;

    static void HandleSystemFailures( Job * job, int result, const AString & stdOut, const AString & stdErr );
    bool ShouldUseDeoptimization() const;
    friend class Client;
    bool ShouldUseCache() const;
    ArgsResponseFileMode GetResponseFileMode() const;
    bool GetVBCCPreprocessedOutput( ConstMemoryStream & outStream ) const;

    void DoClangUnityFixup( Job * job ) const;

    void CreateDriver( ObjectNode::CompilerFlags flags,
                       const AString & remoteSourceRoot,
                       UniquePtr<CompilerDriverBase, DeleteDeletor> & outDriver ) const;

    friend class FunctionObjectList;

    class CompileHelper
    {
    public:
        explicit CompileHelper( bool handleOutput = true, const volatile bool * abort = nullptr );
        ~CompileHelper();

        // start compilation
        bool SpawnCompiler( Job * job,
                            const AString & name,
                            const CompilerNode * compilerNode,
                            const AString & compiler,
                            const Args & fullArgs,
                            const char * workingDir = nullptr );

        // determine overall result
        inline int                      GetResult() const { return m_Result; }

        // access output/error
        inline const AString &          GetOut() const { return m_Out; }
        inline const AString &          GetErr() const { return m_Err; }
        inline bool                     HasAborted() const { return m_Process.HasAborted(); }

    private:
        bool            m_HandleOutput;
        Process         m_Process;
        AString         m_Out;
        AString         m_Err;
        int             m_Result;
    };

    // Exposed Properties
    friend class ObjectListNode;
    AString             m_Compiler;
    AString             m_CompilerOptions;
    AString             m_CompilerOptionsDeoptimized;
    AString             m_CompilerInputFile;
    AString             m_CompilerOutputExtension;
    AString             m_PCHObjectFileName;
    bool                m_DeoptimizeWritableFiles           = false;
    bool                m_DeoptimizeWritableFilesWithToken  = false;
    bool                m_AllowDistribution                 = true;
    bool                m_AllowCaching                      = true;
    Array< AString >    m_CompilerForceUsing;
    AString             m_Preprocessor;
    AString             m_PreprocessorOptions;
    Array< AString >    m_PreBuildDependencyNames;

    // Internal State
    AString             m_PrecompiledHeader;
    CompilerFlags       m_CompilerFlags;
    CompilerFlags       m_PreprocessorFlags;
    uint64_t            m_PCHCacheKey                       = 0;
    uint64_t            m_LightCacheKey                     = 0;
    AString             m_OwnerObjectList; // TODO:C This could be a pointer to the node in the future

    // Not serialized
    Array< AString >    m_Includes;
    bool                m_Remote                            = false;
};

//------------------------------------------------------------------------------
