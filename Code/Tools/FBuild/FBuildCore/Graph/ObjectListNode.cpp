// ObjectListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ObjectListNode.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ObjectListNode, Node, MetaNone() )
    REFLECT( m_Compiler,                            "Compiler",                         MetaFile() )
    REFLECT( m_CompilerOptions,                     "CompilerOptions",                  MetaNone() )
    REFLECT( m_CompilerOptionsDeoptimized,          "CompilerOptionsDeoptimized",       MetaOptional() )
    REFLECT( m_CompilerOutputPath,                  "CompilerOutputPath",               MetaPath() )
    REFLECT( m_CompilerOutputPrefix,                "CompilerOutputPrefix",             MetaOptional() )
    REFLECT( m_CompilerOutputExtension,             "CompilerOutputExtension",          MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputPath,             "CompilerInputPath",                MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerInputPattern,          "CompilerInputPattern",             MetaOptional() )
    REFLECT( m_CompilerInputPathRecurse,            "CompilerInputPathRecurse",         MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputExcludePath,      "CompilerInputExcludePath",         MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerInputExcludedFiles,    "CompilerInputExcludedFiles",       MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY( m_CompilerInputExcludePattern,   "CompilerInputExcludePattern",      MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputUnity,            "CompilerInputUnity",               MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputFiles,            "CompilerInputFiles",               MetaOptional() + MetaFile() )
    REFLECT( m_CompilerInputFilesRoot,              "CompilerInputFilesRoot",           MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerForceUsing,            "CompilerForceUsing",               MetaOptional() + MetaFile() )
    REFLECT( m_DeoptimizeWritableFiles,             "DeoptimizeWritableFiles",          MetaOptional() )
    REFLECT( m_DeoptimizeWritableFilesWithToken,    "DeoptimizeWritableFilesWithToken", MetaOptional() )
    REFLECT( m_AllowDistribution,                   "AllowDistribution",                MetaOptional() )
    REFLECT( m_AllowCaching,                        "AllowCaching",                     MetaOptional() )
    // Precompiled Headers
    REFLECT( m_PCHInputFile,                        "PCHInputFile",                     MetaOptional() + MetaFile() )
    REFLECT( m_PCHOutputFile,                       "PCHOutputFile",                    MetaOptional() + MetaFile() )
    REFLECT( m_PCHOptions,                          "PCHOptions",                       MetaOptional() )
    // Preprocessor
    REFLECT( m_Preprocessor,                        "Preprocessor",                     MetaOptional() + MetaFile() )
    REFLECT( m_PreprocessorOptions,                 "PreprocessorOptions",              MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames,       "PreBuildDependencies",             MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT( m_ExtraPDBPath,                        "ExtraPDBPath",                     MetaHidden() )
    REFLECT( m_ExtraASMPath,                        "ExtraASMPath",                     MetaHidden() )
    REFLECT( m_ObjectListInputStartIndex,           "ObjectListInputStartIndex",        MetaHidden() )
    REFLECT( m_ObjectListInputEndIndex,             "ObjectListInputEndIndex",          MetaHidden() )
    REFLECT( m_NumCompilerInputUnity,               "NumCompilerInputUnity",            MetaHidden() )
    REFLECT( m_NumCompilerInputFiles,               "NumCompilerInputFiles",            MetaHidden() )
REFLECT_END( ObjectListNode )

// ObjectListNode
//------------------------------------------------------------------------------
ObjectListNode::ObjectListNode()
: Node( AString::GetEmpty(), Node::OBJECT_LIST_NODE, Node::FLAG_NONE )
{
    m_LastBuildTimeMs = 10000;

    m_CompilerInputPattern.Append( AStackString<>( "*.cpp" ) );
}

// Initialize
//------------------------------------------------------------------------------
bool ObjectListNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .Compiler
    // TODO:C move GetCompilerNode into ObjectListNode
    CompilerNode * compilerNode( nullptr );
    if ( !((FunctionObjectList *)function)->GetCompilerNode( nodeGraph, iter, m_Compiler, compilerNode ) )
    {
        return false; // GetCompilerNode will have emitted an error
    }

    // .CompilerForceUsing
    // (ObjectListNode doesn't need to depend on this, but we want to check it so that
    //  we can raise errors during parsing instead of during the build when ObjectNode might be created)
    Dependencies compilerForceUsing;
    if ( !function->GetFileNodes( nodeGraph, iter, m_CompilerForceUsing, ".CompilerForceUsing", compilerForceUsing ) )
    {
        return false; // GetFileNode will have emitted an error
    }

    // Check Deoptimized compiler options which are conditionally not optional
    if ( ( m_DeoptimizeWritableFiles || m_DeoptimizeWritableFilesWithToken ) && m_CompilerOptionsDeoptimized.IsEmpty() )
    {
        Error::Error_1101_MissingProperty( iter, function, AStackString<>( ".CompilerOptionsDeoptimized" ) );
        return false;
    }

    // .PCHInputFile
    const bool usingPCH = ( m_PCHInputFile.IsEmpty() == false );
    if ( usingPCH )
    {
        // .PCHOutput and .PCHOptions are required is .PCHInputFile is set
        if ( m_PCHOutputFile.IsEmpty() || m_PCHOptions.IsEmpty() )
        {
            Error::Error_1300_MissingPCHArgs( iter, function );
            return false;
        }

        // Determine flags for PCH - TODO:B Move this into ObjectNode::Initialize
        AStackString<> pchObjectName; // TODO:A Use this
        const uint32_t pchFlags = ObjectNode::DetermineFlags( compilerNode, m_PCHOptions, true, false );
        if ( pchFlags & ObjectNode::FLAG_MSVC )
        {
            if ( ((FunctionObjectList *)function)->CheckMSVCPCHFlags( iter, m_CompilerOptions, m_PCHOptions, m_PCHOutputFile, GetObjExtension(), pchObjectName ) == false )
            {
                return false; // CheckMSVCPCHFlags will have emitted an error
            }
        }

        // .PCHOutputFile
        if ( nodeGraph.FindNode( m_PCHOutputFile ) )
        {
            // TODO:C - Allow existing definition if settings are identical for better multi-ObjectList use of PCH
            Error::Error_1301_AlreadyDefinedPCH( iter, function, m_PCHOutputFile.Get() );
            return false;
        }

        m_PrecompiledHeader = CreateObjectNode( nodeGraph, iter, function, pchFlags, 0, m_PCHOptions, AString::GetEmpty(), AString::GetEmpty(), AString::GetEmpty(), m_PCHOutputFile, m_PCHInputFile, pchObjectName );
        if ( m_PrecompiledHeader == nullptr )
        {
            return false; // CreateObjectNode will have emitted an error
        }
    }

    // .CompilerOptions
    const uint32_t objFlags = ObjectNode::DetermineFlags( compilerNode, m_CompilerOptions, false, usingPCH );
    if ( ( objFlags & ObjectNode::FLAG_MSVC ) && ( objFlags & ObjectNode::FLAG_CREATING_PCH ) )
    {
        // must not specify use of precompiled header (must use the PCH specific options)
        Error::Error_1303_PCHCreateOptionOnlyAllowedOnPCH( iter, function, "Yc", "CompilerOptions" );
        return false;
    }
    if ( ((FunctionObjectList *)function)->CheckCompilerOptions( iter, m_CompilerOptions, objFlags ) == false )
    {
        return false; // CheckCompilerOptions will have emitted an error
    }

    // .Preprocessor
    CompilerNode * preprocessorNode( nullptr );
    if ( m_Preprocessor.IsEmpty() == false )
    {
        // get the preprocessor executable
        if ( ((FunctionObjectList *)function)->GetCompilerNode( nodeGraph, iter, m_Preprocessor, preprocessorNode ) == false )
        {
            return false; // GetCompilerNode will have emitted an error
        }
    }

    // .CompilerInputUnity
    Dependencies compilerInputUnity;
    for ( const AString & unity : m_CompilerInputUnity )
    {
        Node * n = nodeGraph.FindNode( unity );
        if ( n == nullptr )
        {
            Error::Error_1104_TargetNotDefined( iter, function, "CompilerInputUnity", unity );
            return false;
        }
        if ( n->GetType() != Node::UNITY_NODE )
        {
            // TODO:B We should support aliases
            Error::Error_1102_UnexpectedType( iter, function, "CompilerInputUnity", unity, n->GetType(), Node::UNITY_NODE );
            return false;
        }
        compilerInputUnity.Append( Dependency( n ) );
    }
    m_NumCompilerInputUnity = (uint32_t)compilerInputUnity.GetSize();

    // .CompilerInputPath
    Dependencies compilerInputPath;
    if ( !function->GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              m_CompilerInputPath,
                                              m_CompilerInputExcludePath,
                                              m_CompilerInputExcludedFiles,
                                              m_CompilerInputExcludePattern,
                                              m_CompilerInputPathRecurse,
                                              &m_CompilerInputPattern,
                                              "CompilerInputPath",
                                              compilerInputPath ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( compilerInputPath.GetSize() == m_CompilerInputPath.GetSize() ); // No need to store count since they should be the same

    // .CompilerInputFiles
    Dependencies compilerInputFiles;
    if ( !function->GetFileNodes( nodeGraph, iter, m_CompilerInputFiles, "CompilerInputFiles", compilerInputFiles ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    m_NumCompilerInputFiles = (uint32_t)compilerInputFiles.GetSize();

    // Extra output paths
    ((FunctionObjectList *)function)->GetExtraOutputPaths( m_CompilerOptions, m_ExtraPDBPath, m_ExtraASMPath );

    // Store dependencies
    m_StaticDependencies.SetCapacity( m_StaticDependencies.GetSize() + 1 + ( preprocessorNode ? 1 : 0 ) + ( m_PrecompiledHeader ? 1 : 0 ) + compilerInputPath.GetSize() + m_NumCompilerInputUnity + m_NumCompilerInputFiles );
    m_StaticDependencies.Append( Dependency( compilerNode ) );
    if ( preprocessorNode )
    {
        m_StaticDependencies.Append( Dependency( preprocessorNode ) );
    }
    if ( m_PrecompiledHeader )
    {
        m_StaticDependencies.Append( Dependency( m_PrecompiledHeader ) );
    }
    m_StaticDependencies.Append( compilerInputPath );
    m_StaticDependencies.Append( compilerInputUnity );
    m_StaticDependencies.Append( compilerInputFiles );
    //m_StaticDependencies.Append( compilerForceUsing ); // NOTE: Deliberately not depending on this

    // Take note of how many things are not to be treated as inputs (the compiler and preprocessor)
    m_ObjectListInputStartIndex += ( 1 + ( preprocessorNode ? 1 : 0 ) + ( m_PrecompiledHeader ? 1 : 0 ) );
    m_ObjectListInputEndIndex = (uint32_t)m_StaticDependencies.GetSize();

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ObjectListNode::~ObjectListNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::IsAFile() const
{
    return false;
}

// GatherDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    (void)forceClean; // dynamic deps are always re-added here, so this is meaningless

    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    #if defined( __WINDOWS__ )
        // On Windows, with MSVC we compile a cpp file to generate the PCH
        // Filter here to ensure that doesn't get compiled twice
        Node * pchCPP = nullptr;
        if ( m_PrecompiledHeader && m_PrecompiledHeader->IsMSVC() )
        {
            pchCPP = m_PrecompiledHeader->GetPrecompiledHeaderCPPFile();
        }
    #endif

    // Handle converting all static inputs into dynamic onces (i.e. cpp->obj)
    for ( size_t i=m_ObjectListInputStartIndex; i<m_ObjectListInputEndIndex; ++i )
    {
        Dependency & dep = m_StaticDependencies[ i ];

        // is this a dir list?
        if ( dep.GetNode()->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            // get the list of files
            DirectoryListNode * dln = dep.GetNode()->CastTo< DirectoryListNode >();
            const Array< FileIO::FileInfo > & files = dln->GetFiles();
            m_DynamicDependencies.SetCapacity( m_DynamicDependencies.GetSize() + files.GetSize() );
            for ( Array< FileIO::FileInfo >::Iter fIt = files.Begin();
                    fIt != files.End();
                    fIt++ )
            {
                // Create the file node (or find an existing one)
                Node * n = nodeGraph.FindNode( fIt->m_Name );
                if ( n == nullptr )
                {
                    n = nodeGraph.CreateFileNode( fIt->m_Name );
                }
                else if ( n->IsAFile() == false )
                {
                    FLOG_ERROR( "Library() .CompilerInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                    return false;
                }

                // ignore the precompiled header as a convenience for the user
                // so they don't have to exclude it explicitly
                #if defined( __WINDOWS__ )
                    if ( n == pchCPP )
                    {
                        continue;
                    }
                #endif

                // create the object that will compile the above file
                if ( CreateDynamicObjectNode( nodeGraph, n, dln->GetPath() ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }
        }
        else if ( dep.GetNode()->GetType() == Node::UNITY_NODE )
        {
            // get the dir list from the unity node
            UnityNode * un = dep.GetNode()->CastTo< UnityNode >();

            // unity files
            const Array< AString > & unityFiles = un->GetUnityFileNames();
            for ( Array< AString >::Iter it = unityFiles.Begin();
                  it != unityFiles.End();
                  it++ )
            {
                Node * n = nodeGraph.FindNode( *it );
                if ( n == nullptr )
                {
                    n = nodeGraph.CreateFileNode( *it );
                }
                else if ( n->IsAFile() == false )
                {
                    FLOG_ERROR( "Library() .CompilerInputUnity '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                    return false;
                }

                // create the object that will compile the above file
                if ( CreateDynamicObjectNode( nodeGraph, n, AString::GetEmpty(), true ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }

            // files from unity to build individually
            const Array< UnityNode::FileAndOrigin > & isolatedFiles = un->GetIsolatedFileNames();
            for ( Array< UnityNode::FileAndOrigin >::Iter it = isolatedFiles.Begin();
                  it != isolatedFiles.End();
                  it++ )
            {
                Node * n = nodeGraph.FindNode( it->GetName() );
                if ( n == nullptr )
                {
                    n = nodeGraph.CreateFileNode( it->GetName() );
                }
                else if ( n->IsAFile() == false )
                {
                    FLOG_ERROR( "Library() Isolated '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                    return false;
                }

                // create the object that will compile the above file
                const AString & baseDir = it->GetDirListOrigin() ? it->GetDirListOrigin()->GetPath() : AString::GetEmpty();
                if ( CreateDynamicObjectNode( nodeGraph, n, baseDir, false, true ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }
        }
        else if ( dep.GetNode()->IsAFile() )
        {
            // a single file, create the object that will compile it
            if ( CreateDynamicObjectNode( nodeGraph, dep.GetNode(), AString::GetEmpty() ) == false )
            {
                return false; // CreateDynamicObjectNode will have emitted error
            }
        }
        else
        {
            ASSERT( false ); // unexpected node type
        }
    }

    // If we have a precompiled header, add that to our dynamic deps so that
    // any symbols in the PCH's .obj are also linked, when either:
    // a) we are a static library
    // b) a DLL or executable links our .obj files
    if ( m_PrecompiledHeader )
    {
        m_DynamicDependencies.Append( Dependency( m_PrecompiledHeader ) );
    }

    return true;
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    if ( GatherDynamicDependencies( nodeGraph, forceClean ) == false )
    {
        return false; // GatherDynamicDependencies will have emitted error
    }

    // make sure we have something to build!
    if ( m_DynamicDependencies.GetSize() == 0 )
    {
        FLOG_ERROR( "No files found to build '%s'", GetName().Get() );
        return false;
    }

    if ( m_ExtraASMPath.IsEmpty() == false )
    {
        if ( !FileIO::EnsurePathExists( m_ExtraASMPath ) )
        {
            FLOG_ERROR( "Failed to create folder for .asm file '%s'", m_ExtraASMPath.Get() );
            return false;
        }
    }

    if ( m_ExtraPDBPath.IsEmpty() == false )
    {
        if ( !FileIO::EnsurePathExists( m_ExtraPDBPath ) )
        {
            FLOG_ERROR( "Failed to create folder for .pdb file '%s'", m_ExtraPDBPath.Get() );
            return false;
        }
    }

    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectListNode::DoBuild( Job * UNUSED( job ) )
{
    // consider ourselves to be as recent as the newest file
    uint64_t timeStamp = 0;
    const Dependency * const end = m_DynamicDependencies.End();
    for ( const Dependency * it = m_DynamicDependencies.Begin(); it != end; ++it )
    {
        ObjectNode * on = it->GetNode()->CastTo< ObjectNode >();
        timeStamp = Math::Max< uint64_t >( timeStamp, on->GetStamp() );
    }
    m_Stamp = timeStamp;

    return NODE_RESULT_OK;
}

// GetInputFiles
//------------------------------------------------------------------------------
void ObjectListNode::GetInputFiles( Args & fullArgs, const AString & pre, const AString & post, bool objectsInsteadOfLibs ) const
{
    for ( Dependencies::Iter i = m_DynamicDependencies.Begin();
          i != m_DynamicDependencies.End();
          i++ )
    {
        const Node * n = i->GetNode();

        // handle pch files - get path to object
        if ( n->GetType() == Node::OBJECT_NODE )
        {
            // handle pch files - get path to matching object
            const ObjectNode * on = n->CastTo< ObjectNode >();
            if ( on->IsCreatingPCH() )
            {
                if ( on->IsMSVC() )
                {
                    fullArgs += pre;
                    fullArgs += on->GetName();
                    fullArgs += on->GetObjExtension();
                    fullArgs += post;
                    fullArgs.AddDelimiter();
                    continue;
                }
                else
                {
                    // Clang/GCC/SNC don't have an object to link for a pch
                    continue;
                }
            }
        }

        // extract objects from additional lists
        if ( n->GetType() == Node::OBJECT_LIST_NODE )
        {
            ASSERT( GetType() == Node::LIBRARY_NODE ); // should only be possible for a LibraryNode

            // insert all the objects in the object list
            ObjectListNode * oln = n->CastTo< ObjectListNode >();
            oln->GetInputFiles( fullArgs, pre, post, objectsInsteadOfLibs );
            continue;
        }

        // get objects used to create libs
        if ( ( n->GetType() == Node::LIBRARY_NODE ) && objectsInsteadOfLibs )
        {
            ASSERT( GetType() == Node::LIBRARY_NODE ); // should only be possible for a LibraryNode

            // insert all the objects in the object list
            LibraryNode * ln = n->CastTo< LibraryNode >();
            ln->GetInputFiles( fullArgs, pre, post, objectsInsteadOfLibs );
            continue;
        }

        // normal object
        fullArgs += pre;
        fullArgs += n->GetName();
        fullArgs += post;
        fullArgs.AddDelimiter();
    }
}

// GetInputFiles
//------------------------------------------------------------------------------
void ObjectListNode::GetInputFiles( Array< AString > & files ) const
{
    // only valid to call on ObjectListNode (not LibraryNode)
    ASSERT( GetType() == Node::OBJECT_LIST_NODE );

    files.SetCapacity( files.GetCapacity() + m_DynamicDependencies.GetSize() );
    for ( Dependencies::Iter i = m_DynamicDependencies.Begin();
          i != m_DynamicDependencies.End();
          i++ )
    {
        const Node * n = i->GetNode();
        files.Append( n->GetName() );
    }
}

// GetCompiler
//------------------------------------------------------------------------------
CompilerNode * ObjectListNode::GetCompiler() const
{
    size_t compilerIndex = 0;
    if ( GetType() == Node::LIBRARY_NODE )
    {
        // Libraries store the Librarian at index 0
        ++compilerIndex;
    }
    return m_StaticDependencies[ compilerIndex ].GetNode()->CastTo< CompilerNode >();
}

// GetPreprocessor
//------------------------------------------------------------------------------
CompilerNode * ObjectListNode::GetPreprocessor() const
{
    // Do we have a preprocessor?
    if ( m_Preprocessor.IsEmpty() )
    {
        return nullptr;
    }

    size_t preprocessorIndex = 1;
    if ( GetType() == Node::LIBRARY_NODE )
    {
        // Libraries store the Librarian at index 0
        ++preprocessorIndex;
    }

    return m_StaticDependencies[ preprocessorIndex ].GetNode()->CastTo< CompilerNode >();
}

// CreateDynamicObjectNode
//------------------------------------------------------------------------------
bool ObjectListNode::CreateDynamicObjectNode( NodeGraph & nodeGraph, Node * inputFile, const AString & baseDir, bool isUnityNode, bool isIsolatedFromUnityNode )
{
    const AString & fileName = inputFile->GetName();

    // Transform src file to dst object path
    // get file name only (no path, no ext)
    const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
    lastSlash = lastSlash ? ( lastSlash + 1 ) : fileName.Get();
    const char * lastDot = fileName.FindLast( '.' );
    lastDot = lastDot && ( lastDot > lastSlash ) ? lastDot : fileName.GetEnd();

    // if source comes from a directory listing, use path relative to dirlist base
    // to replicate the folder hierearchy in the output
    AStackString<> subPath;
    if ( baseDir.IsEmpty() == false )
    {
        ASSERT( NodeGraph::IsCleanPath( baseDir ) );
        if ( PathUtils::PathBeginsWith( fileName, baseDir ) )
        {
            // ... use everything after that
            subPath.Assign( fileName.Get() + baseDir.GetLength(), lastSlash ); // includes last slash
        }
    }
    else
    {
        if ( !m_CompilerInputFilesRoot.IsEmpty() && PathUtils::PathBeginsWith( fileName, m_CompilerInputFilesRoot ) )
        {
            // ... use everything after that
            subPath.Assign( fileName.Get() + m_CompilerInputFilesRoot.GetLength(), lastSlash ); // includes last slash
        }
    }

    AStackString<> fileNameOnly( lastSlash, lastDot );
    AStackString<> objFile( m_CompilerOutputPath );
    objFile += subPath;
    objFile += m_CompilerOutputPrefix;
    objFile += fileNameOnly;
    objFile += GetObjExtension();

    // Create an ObjectNode to compile the above file
    // and depend on that
    Node * on = nodeGraph.FindNode( objFile );
    if ( on == nullptr )
    {
        // determine flags - TODO:B Move DetermineFlags call out of build-time
        const bool usingPCH = ( m_PCHInputFile.IsEmpty() == false );
        uint32_t flags = ObjectNode::DetermineFlags( GetCompiler(), m_CompilerOptions, false, usingPCH );
        if ( isUnityNode )
        {
            flags |= ObjectNode::FLAG_UNITY;
        }
        if ( isIsolatedFromUnityNode )
        {
            flags |= ObjectNode::FLAG_ISOLATED_FROM_UNITY;
        }
        uint32_t preprocessorFlags = 0;
        if ( m_Preprocessor.IsEmpty() == false )
        {
            // determine flags - TODO:B Move DetermineFlags call out of build-time
            preprocessorFlags = ObjectNode::DetermineFlags( GetPreprocessor(), m_PreprocessorOptions, false, usingPCH );
        }

        BFFIterator dummyIter;
        ObjectNode * objectNode = CreateObjectNode( nodeGraph, dummyIter, nullptr, flags, preprocessorFlags, m_CompilerOptions, m_CompilerOptionsDeoptimized, m_Preprocessor, m_PreprocessorOptions, objFile, inputFile->GetName(), AString::GetEmpty() );
        if ( !objectNode )
        {
            FLOG_ERROR( "Failed to create node '%s'!", objFile.Get() );
            return false;
        }
        on = objectNode;
    }
    else if ( on->GetType() != Node::OBJECT_NODE )
    {
        FLOG_ERROR( "Node '%s' is not an ObjectNode (type: %s)", on->GetName().Get(), on->GetTypeName() );
        return false;
    }
    else
    {
        ObjectNode * other = on->CastTo< ObjectNode >();
        if ( inputFile != other->GetSourceFile() )
        {
            FLOG_ERROR( "Conflicting objects found:\n"
                        " File A: %s\n"
                        " File B: %s\n"
                        " Both compile to: %s\n",
                        inputFile->GetName().Get(),
                        other->GetSourceFile()->GetName().Get(),
                        objFile.Get() );
            return false;
        }
    }
    m_DynamicDependencies.Append( Dependency( on ) );
    return true;
}

// CreateObjectNode
//------------------------------------------------------------------------------
ObjectNode * ObjectListNode::CreateObjectNode( NodeGraph & nodeGraph,
                                               const BFFIterator & iter,
                                               const Function * function,
                                               const uint32_t flags,
                                               const uint32_t preprocessorFlags,
                                               const AString & compilerOptions,
                                               const AString & compilerOptionsDeoptimized,
                                               const AString & preprocessor,
                                               const AString & preprocessorOptions,
                                               const AString & objectName,
                                               const AString & objectInput,
                                               const AString & pchObjectName )
{
    ObjectNode * node= nodeGraph.CreateObjectNode( objectName );
    node->m_Compiler = m_Compiler;
    node->m_CompilerOptions = compilerOptions;
    node->m_CompilerOptionsDeoptimized = compilerOptionsDeoptimized;
    node->m_CompilerInputFile = objectInput;
    node->m_CompilerOutputExtension = m_CompilerOutputExtension;
    node->m_PCHObjectFileName = pchObjectName;
    if ( flags & ObjectNode::FLAG_CREATING_PCH )
    {
        // Precompiled headers are never de-optimized
        node->m_DeoptimizeWritableFiles = false;
        node->m_DeoptimizeWritableFilesWithToken = false;
    }
    else
    {
        node->m_DeoptimizeWritableFiles = m_DeoptimizeWritableFiles;
        node->m_DeoptimizeWritableFilesWithToken = m_DeoptimizeWritableFilesWithToken;
    }
    node->m_AllowDistribution = m_AllowDistribution;
    node->m_AllowCaching = m_AllowCaching;
    node->m_CompilerForceUsing = m_CompilerForceUsing;
    node->m_PreBuildDependencyNames = m_PreBuildDependencyNames;
    node->m_PrecompiledHeader = m_PrecompiledHeader;
    node->m_Preprocessor = preprocessor;
    node->m_PreprocessorOptions = preprocessorOptions;
    node->m_Flags = flags;
    node->m_PreprocessorFlags = preprocessorFlags;

    if ( !node->Initialize( nodeGraph, iter, function ) )
    {
        // TODO:A We have a node in the graph which is in an invalid state
        return nullptr; // Initialize will have emitted an error
    }
    return node;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ObjectListNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    ObjectListNode * node = nodeGraph.CreateObjectListNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }

    // TODO:C Handle through normal serialization
    NODE_LOAD_NODE_LINK( Node, precompiledHeader );
    node->m_PrecompiledHeader = precompiledHeader ? precompiledHeader->CastTo< ObjectNode >() : nullptr;

    return node;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void ObjectListNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );

    // TODO:C Handle through normal serialization
    NODE_SAVE_NODE_LINK( m_PrecompiledHeader );
}

// GetObjExtension
//------------------------------------------------------------------------------
const char * ObjectListNode::GetObjExtension() const
{
    if ( m_CompilerOutputExtension.IsEmpty() )
    {
        #if defined( __WINDOWS__ )
            return ".obj";
        #else
            return ".o";
        #endif
    }
    return m_CompilerOutputExtension.Get();
}

//------------------------------------------------------------------------------
