// ObjectNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Assert.h"
#include "Core/Process/Process.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class BFFIterator;
class ConstMemoryStream;
class Function;
class NodeGraph;
class NodeProxy;
class ObjectNode;

// ObjectNode
//------------------------------------------------------------------------------
class ObjectNode : public FileNode
{
    REFLECT_NODE_DECLARE( ObjectNode )
public:
    ObjectNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    // simplified remote constructor
    explicit ObjectNode( const AString & objectName,
                         NodeProxy * srcFile,
                         const AString & compilerOptions,
                         uint32_t flags );
    virtual ~ObjectNode();

    static inline Node::Type GetTypeS() { return Node::OBJECT_NODE; }


    enum Flags
    {
        FLAG_CAN_BE_CACHED      =   0x01,
        FLAG_CAN_BE_DISTRIBUTED =   0x02,
        FLAG_USING_PCH          =   0x04,
        FLAG_GCC                =   0x10,
        FLAG_MSVC               =   0x20,
        FLAG_CREATING_PCH       =   0x40,
        FLAG_SNC                =   0x80,
        FLAG_USING_CLR          =   0x100,
        FLAG_CLANG              =   0x200,
        FLAG_UNITY              =   0x400,
        FLAG_ISOLATED_FROM_UNITY=   0x800,
        FLAG_USING_PDB          =   0x1000,
        CODEWARRIOR_WII         =   0x2000,
        GREENHILLS_WIIU         =   0x4000,
        FLAG_CUDA_NVCC          =   0x10000,
        FLAG_INCLUDES_IN_STDERR =   0x20000,
        FLAG_QT_RCC             =   0x40000,
        FLAG_WARNINGS_AS_ERRORS_MSVC    = 0x80000,
        FLAG_VBCC               =   0x100000
    };
    static uint32_t DetermineFlags( const CompilerNode * compilerNode,
                                    const AString & args,
                                    bool creatingPCH,
                                    bool usingPCH );
    static bool IsCompilerArg_MSVC( const AString & token, const char * arg );
    static bool IsStartOfCompilerArg_MSVC( const AString & token, const char * arg );

    inline bool IsCreatingPCH() const { return GetFlag( FLAG_CREATING_PCH ); }
    inline bool IsUsingPCH() const { return GetFlag( FLAG_USING_PCH ); }
    inline bool IsMSVC() const { return GetFlag( FLAG_MSVC ); }
    inline bool IsUsingPDB() const { return GetFlag( FLAG_USING_PDB ); }

    virtual void Save( IOStream & stream ) const override;
    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );

    virtual void SaveRemote( IOStream & stream ) const override;
    static Node * LoadRemote( IOStream & stream );

    inline Node * GetCompiler() const { return m_StaticDependencies[ 0 ].GetNode(); }
    inline Node * GetSourceFile() const { return m_StaticDependencies[ 1 ].GetNode(); }
    Node * GetDedicatedPreprocessor() const;
    #if defined( __WINDOWS__ )
        inline Node * GetPrecompiledHeaderCPPFile() const { ASSERT( GetFlag( FLAG_CREATING_PCH ) ); return m_StaticDependencies[ 1 ].GetNode(); }
    #endif

    void GetPDBName( AString & pdbName ) const;

    const char * GetObjExtension() const;
private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob ) override;
    virtual bool Finalize( NodeGraph & nodeGraph ) override;

    BuildResult DoBuildMSCL_NoCache( Job * job, bool useDeoptimization );
    BuildResult DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache, bool useSimpleDist );
    BuildResult DoBuildWithPreProcessor2( Job * job, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob );
    BuildResult DoBuild_QtRCC( Job * job );
    BuildResult DoBuildOther( Job * job, bool useDeoptimization );

    bool ProcessIncludesMSCL( const char * output, uint32_t outputSize );
    bool ProcessIncludesWithPreProcessor( Job * job );

    const AString & GetCacheName( Job * job ) const;
    bool RetrieveFromCache( Job * job );
    void WriteToCache( Job * job );
    bool GetExtraCacheFilePath( const Job * job, AString & extraFileName ) const;

    void EmitCompilationMessage( const Args & fullArgs, bool useDeoptimization, bool stealingRemoteJob = false, bool racingRemoteJob = false, bool useDedicatedPreprocessor = false, bool isRemote = false ) const;

    enum Pass
    {
        PASS_PREPROCESSOR_ONLY,
        PASS_COMPILE_PREPROCESSED,
        PASS_COMPILE,
        PASS_PREP_FOR_SIMPLE_DISTRIBUTION,
    };
    static bool StripTokenWithArg( const char * tokenToCheckFor, const AString & token, size_t & index );
    static bool StripTokenWithArg_MSVC( const char * tokenToCheckFor, const AString & token, size_t & index );
    static bool StripToken( const char * tokenToCheckFor, const AString & token, bool allowStartsWith = false );
    static bool StripToken_MSVC( const char * tokenToCheckFor, const AString & token, bool allowStartsWith = false );
    bool BuildArgs( const Job * job, Args & fullArgs, Pass pass, bool useDeoptimization, bool useShowIncludes, const AString & overrideSrcFile = AString::GetEmpty() ) const;

    void ExpandCompilerForceUsing( Args & fullArgs, const AString & pre, const AString & post ) const;
    bool BuildPreprocessedOutput( const Args & fullArgs, Job * job, bool useDeoptimization ) const;
    bool LoadStaticSourceFileForDistribution( const Args & fullArgs, Job * job, bool useDeoptimization ) const;
    void TransferPreprocessedData( const char * data, size_t dataSize, Job * job ) const;
    bool WriteTmpFile( Job * job, AString & tmpDirectory, AString & tmpFileName ) const;
    bool BuildFinalOutput( Job * job, const Args & fullArgs ) const;

    inline bool GetFlag( uint32_t flag ) const { return ( ( m_Flags & flag ) != 0 ); }
    inline bool GetPreprocessorFlag( uint32_t flag ) const { return ( ( m_PreprocessorFlags & flag ) != 0 ); }

    static void HandleSystemFailures( Job * job, int result, const char * stdOut, const char * stdErr );
    bool ShouldUseDeoptimization() const;
    friend class Client;
    bool ShouldUseCache() const;
    bool CanUseResponseFile() const;
    bool GetVBCCPreprocessedOutput( ConstMemoryStream & outStream ) const;

    friend class FunctionObjectList;

    class CompileHelper
    {
    public:
        explicit CompileHelper( bool handleOutput = true, const volatile bool * abort = nullptr );
        ~CompileHelper();

        // start compilation
        bool SpawnCompiler( Job * job, const AString & name, const AString & compiler, const Args & fullArgs, const char * workingDir = nullptr );

        // determine overall result
        inline int                      GetResult() const { return m_Result; }

        // access output/error
        inline const AutoPtr< char > &  GetOut() const { return m_Out; }
        inline uint32_t                 GetOutSize() const { return m_OutSize; }
        inline const AutoPtr< char > &  GetErr() const { return m_Err; }
        inline uint32_t                 GetErrSize() const { return m_ErrSize; }
        inline bool                     HasAborted() const { return m_Process.HasAborted(); }

    private:
        bool            m_HandleOutput;
        Process         m_Process;
        AutoPtr< char > m_Out;
        uint32_t        m_OutSize;
        AutoPtr< char > m_Err;
        uint32_t        m_ErrSize;
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
    ObjectNode *        m_PrecompiledHeader                 = nullptr;
    uint32_t            m_Flags                             = 0;
    uint32_t            m_PreprocessorFlags                 = 0;
    uint64_t            m_PCHCacheKey                       = 0;

    // Not serialized
    Array< AString >    m_Includes;
    bool                m_Remote                            = false;
};

//------------------------------------------------------------------------------
