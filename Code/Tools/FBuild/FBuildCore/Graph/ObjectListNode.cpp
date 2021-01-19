// ObjectListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ObjectListNode.h"

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
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ObjectListNode, Node, MetaNone() )
    REFLECT( m_Compiler,                            "Compiler",                         MetaFile() + MetaAllowNonFile() )
    REFLECT( m_CompilerOptions,                     "CompilerOptions",                  MetaNone() )
    REFLECT( m_CompilerOptionsDeoptimized,          "CompilerOptionsDeoptimized",       MetaOptional() )
    REFLECT( m_CompilerOutputPath,                  "CompilerOutputPath",               MetaOptional() + MetaPath() )
    REFLECT( m_CompilerOutputPrefix,                "CompilerOutputPrefix",             MetaOptional() )
    REFLECT( m_CompilerOutputExtension,             "CompilerOutputExtension",          MetaOptional() )
    REFLECT( m_CompilerOutputKeepBaseExtension,     "CompilerOutputKeepBaseExtension",  MetaOptional() )
    REFLECT( m_CompilerInputAllowNoFiles,           "CompilerInputAllowNoFiles",        MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputPath,             "CompilerInputPath",                MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerInputPattern,          "CompilerInputPattern",             MetaOptional() )
    REFLECT( m_CompilerInputPathRecurse,            "CompilerInputPathRecurse",         MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputExcludePath,      "CompilerInputExcludePath",         MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerInputExcludedFiles,    "CompilerInputExcludedFiles",       MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY( m_CompilerInputExcludePattern,   "CompilerInputExcludePattern",      MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY( m_CompilerInputUnity,            "CompilerInputUnity",               MetaOptional() )
    REFLECT_ARRAY( m_CompilerInputFiles,            "CompilerInputFiles",               MetaOptional() + MetaFile() )
    REFLECT( m_CompilerInputFilesRoot,              "CompilerInputFilesRoot",           MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CompilerInputObjectLists,      "CompilerInputObjectLists",         MetaOptional() )
    REFLECT_ARRAY( m_CompilerForceUsing,            "CompilerForceUsing",               MetaOptional() + MetaFile() )
    REFLECT( m_DeoptimizeWritableFiles,             "DeoptimizeWritableFiles",          MetaOptional() )
    REFLECT( m_DeoptimizeWritableFilesWithToken,    "DeoptimizeWritableFilesWithToken", MetaOptional() )
    REFLECT( m_AllowDistribution,                   "AllowDistribution",                MetaOptional() )
    REFLECT( m_AllowCaching,                        "AllowCaching",                     MetaOptional() )
    REFLECT( m_Hidden,                              "Hidden",                           MetaOptional() )
    // Precompiled Headers
    REFLECT( m_PCHInputFile,                        "PCHInputFile",                     MetaOptional() + MetaFile() )
    REFLECT( m_PCHOutputFile,                       "PCHOutputFile",                    MetaOptional() + MetaFile() )
    REFLECT( m_PCHOptions,                          "PCHOptions",                       MetaOptional() )
    // Preprocessor
    REFLECT( m_Preprocessor,                        "Preprocessor",                     MetaOptional() + MetaFile() + MetaAllowNonFile() )
    REFLECT( m_PreprocessorOptions,                 "PreprocessorOptions",              MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames,       "PreBuildDependencies",             MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT( m_PrecompiledHeaderName,               "PrecompiledHeaderName",            MetaHidden() )
    #if defined( __WINDOWS__ )
        REFLECT( m_PrecompiledHeaderCPPFile,            "PrecompiledHeaderCPPFile",         MetaHidden() )
    #endif
    REFLECT( m_ExtraPDBPath,                        "ExtraPDBPath",                     MetaHidden() )
    REFLECT( m_ExtraASMPath,                        "ExtraASMPath",                     MetaHidden() )
    REFLECT( m_ObjectListInputStartIndex,           "ObjectListInputStartIndex",        MetaHidden() )
    REFLECT( m_ObjectListInputEndIndex,             "ObjectListInputEndIndex",          MetaHidden() )
    REFLECT( m_ObjFlags,                            "ObjFlags",                         MetaHidden() )
    REFLECT( m_ObjFlagsPreprocessor,                "ObjFlagsPreprocessor",             MetaHidden() )
REFLECT_END( ObjectListNode )

// ObjectListNode
//------------------------------------------------------------------------------
ObjectListNode::ObjectListNode()
: Node( AString::GetEmpty(), Node::OBJECT_LIST_NODE, Node::FLAG_NONE )
{
    m_LastBuildTimeMs = 10000;

    m_CompilerInputPattern.EmplaceBack( "*.cpp" );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .Compiler
    CompilerNode * compilerNode( nullptr );
    if ( !Function::GetCompilerNode( nodeGraph, iter, function, m_Compiler, compilerNode ) )
    {
        return false; // GetCompilerNode will have emitted an error
    }

    // Check for CSharp (this must use CSAssembly not ObjectList)
    if ( compilerNode->GetCompilerFamily() == CompilerNode::CompilerFamily::CSHARP )
    {
        Error::Error_1503_CSharpCompilerShouldUseCSAssembly( iter, function );
        return false;
    }

    // .Preprocessor
    CompilerNode * preprocessorNode( nullptr );
    if ( m_Preprocessor.IsEmpty() == false )
    {
        // get the preprocessor executable
        if ( Function::GetCompilerNode( nodeGraph, iter, function, m_Preprocessor, preprocessorNode ) == false )
        {
            return false; // GetCompilerNode will have emitted an error
        }
    }

    // .CompilerForceUsing
    // (ObjectListNode doesn't need to depend on this, but we want to check it so that
    //  we can raise errors during parsing instead of during the build when ObjectNode might be created)
    Dependencies compilerForceUsing;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_CompilerForceUsing, ".CompilerForceUsing", compilerForceUsing ) )
    {
        return false; // GetFileNode will have emitted an error
    }

    // Check Deoptimized compiler options which are conditionally not optional
    if ( ( m_DeoptimizeWritableFiles || m_DeoptimizeWritableFilesWithToken ) && m_CompilerOptionsDeoptimized.IsEmpty() )
    {
        Error::Error_1101_MissingProperty( iter, function, AStackString<>( ".CompilerOptionsDeoptimized" ) );
        return false;
    }

    // Creating a PCH?
    const bool creatingPCH = ( m_PCHInputFile.IsEmpty() == false );
    ObjectNode * precompiledHeader = nullptr;
    if ( creatingPCH )
    {
        // .PCHOptions are required to create PCH
        if ( m_PCHOutputFile.IsEmpty() || m_PCHOptions.IsEmpty() )
        {
            Error::Error_1300_MissingPCHArgs( iter, function );
            return false;
        }

        // Check PCH creation command line options
        AStackString<> pchObjectName; // TODO:A Use this
        const uint32_t pchFlags = ObjectNode::DetermineFlags( compilerNode, m_PCHOptions, true, false );
        if ( pchFlags & ( ObjectNode::FLAG_MSVC | ObjectNode::FLAG_CLANG_CL ) )
        {
            if ( ((FunctionObjectList *)function)->CheckMSVCPCHFlags_Create( iter, m_PCHOptions, m_PCHOutputFile, GetObjExtension(), pchObjectName ) == false )
            {
                return false; // CheckMSVCPCHFlags_Create will have emitted an error
            }
        }

        // PCH can be shared between ObjectLists, but must only be defined once
        if ( nodeGraph.FindNode( m_PCHOutputFile ) )
        {
            Error::Error_1301_AlreadyDefinedPCH( iter, function, m_PCHOutputFile.Get() );
            return false;
        }

        // Create the PCH node
        precompiledHeader = CreateObjectNode( nodeGraph, iter, function, pchFlags, 0, m_PCHOptions, AString::GetEmpty(), AString::GetEmpty(), AString::GetEmpty(), m_PCHOutputFile, m_PCHInputFile, pchObjectName );
        if ( precompiledHeader == nullptr )
        {
            return false; // CreateObjectNode will have emitted an error
        }
    }

    // Are we compiling and files?
    const bool compilingFiles = ( ( m_CompilerInputPath.IsEmpty() == false ) ||
                                  ( m_CompilerInputFiles.IsEmpty() == false ) ||
                                  ( m_CompilerInputUnity.IsEmpty() == false ) ||
                                  ( m_CompilerInputObjectLists.IsEmpty() == false ) );
    if ( compilingFiles )
    {
        // Using a PCH?
        const bool usingPCH = ( m_PCHOutputFile.IsEmpty() == false );

        // Cache flags for compiler and preprocessor
        m_ObjFlags = ObjectNode::DetermineFlags( compilerNode, m_CompilerOptions, false, usingPCH );
        m_ObjFlagsPreprocessor = preprocessorNode ? ObjectNode::DetermineFlags( preprocessorNode, m_PreprocessorOptions, false, usingPCH ) : 0;

        // Check validity of PCH setup
        if ( usingPCH )
        {
            // Check for correct PCH usage options
            if ( m_ObjFlags & ( ObjectNode::FLAG_MSVC | ObjectNode::FLAG_CLANG_CL ) )
            {
                if ( ((FunctionObjectList *)function)->CheckMSVCPCHFlags_Use( iter, m_CompilerOptions, m_ObjFlags ) == false )
                {
                    return false; // CheckMSVCPCHFlags_Use will have emitted an error
                }
            }

            // Get the PCH node
            if ( creatingPCH )
            {
                ASSERT( precompiledHeader ); // This ObjectList also should have created it above
            }
            else
            {
                // If we are not creating it, we must be re-using it from another object list
                const Node * node = nodeGraph.FindNode( m_PCHOutputFile );
                if ( ( node == nullptr ) ||
                     ( node->GetType() != Node::OBJECT_NODE ) ||
                     ( node->CastTo<ObjectNode>()->GetFlag( ObjectNode::FLAG_CREATING_PCH ) == false ) )
                {
                    // PCH was not defined
                    Error::Error_1104_TargetNotDefined( iter, function, "PCHOutputFile", m_PCHOutputFile );
                    return false;
                }
                precompiledHeader = node->CastTo< ObjectNode >();
            }
        }

        // .CompilerOptions
        if ( ((FunctionObjectList *)function)->CheckCompilerOptions( iter, m_CompilerOptions, m_ObjFlags ) == false )
        {
            return false; // CheckCompilerOptions will have emitted an error
        }

        // .CompilerOutputPath is required when compiling files (not needed if only creating a PCH)
        if ( m_CompilerOutputPath.IsEmpty() )
        {
            Error::Error_1101_MissingProperty( iter, function, AStackString<>( "CompilerOutputPath" ) );
        }
    }

    // Cache precompiled header details
    if ( precompiledHeader )
    {
        m_PrecompiledHeaderName = precompiledHeader->GetName();

        // Cache the CPP file associated with the PCH for MSVC (and Clang in MSVC mode)
        // so that it can be excluded from normal compilation
        #if defined( __WINDOWS__ )
            if ( ( compilerNode->GetCompilerFamily() == CompilerNode::CompilerFamily::MSVC ) ||
                 ( compilerNode->GetCompilerFamily() == CompilerNode::CompilerFamily::CLANG_CL ) )
            {
                m_PrecompiledHeaderCPPFile = precompiledHeader->GetPrecompiledHeaderCPPFile()->GetName();
            }
        #endif
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
        compilerInputUnity.EmplaceBack( n );
    }

    // .CompilerInputPath
    Dependencies compilerInputPath;
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_CompilerInputPath,
                                              m_CompilerInputExcludePath,
                                              m_CompilerInputExcludedFiles,
                                              m_CompilerInputExcludePattern,
                                              m_CompilerInputPathRecurse,
                                              false, // Don't include read-only status in hash
                                              &m_CompilerInputPattern,
                                              "CompilerInputPath",
                                              compilerInputPath ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( compilerInputPath.GetSize() == m_CompilerInputPath.GetSize() ); // No need to store count since they should be the same

    // .CompilerInputFiles
    // NOTE: We get the file nodes to ensure nice errors are reported for undefined
    // targets during bff parsing, but we don't depend on these files (the object nodes
    // we create later depend on these files)
    {
        Dependencies compilerInputFiles;
        if ( !Function::GetFileNodes( nodeGraph, iter, function, m_CompilerInputFiles, "CompilerInputFiles", compilerInputFiles ) )
        {
            return false; // GetFileNode will have emitted an error
        }
    }

    // .CompilerInputObjectLists
    Dependencies compilerInputObjectLists;
    if ( Function::GetObjectListNodes( nodeGraph, iter, function, m_CompilerInputObjectLists, "CompilerInputObjectLists", compilerInputObjectLists ) == false )
    {
        return false; //
    }

    // Extra output paths
    ((FunctionObjectList *)function)->GetExtraOutputPaths( m_CompilerOptions, m_ExtraPDBPath, m_ExtraASMPath );

    // Store dependencies
    m_StaticDependencies.SetCapacity( compilerInputPath.GetSize() +
                                      compilerInputUnity.GetSize() +
                                      compilerInputObjectLists.GetSize() );
    m_StaticDependencies.Append( compilerInputPath );
    m_StaticDependencies.Append( compilerInputUnity );
    m_StaticDependencies.Append( compilerInputObjectLists );

    // Take note of how many things are treated as inputs
    // (this is needed so LibraryNode can add some additional things)
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

    // Handle converting all static inputs into dynamic onces (i.e. cpp->obj)
    for ( size_t i=m_ObjectListInputStartIndex; i<m_ObjectListInputEndIndex; ++i )
    {
        Dependency & dep = m_StaticDependencies[ i ];

        // is this a dir list?
        if ( dep.GetNode()->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            // get the list of files
            const DirectoryListNode * dln = dep.GetNode()->CastTo< DirectoryListNode >();
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
                    if ( n->GetName() == m_PrecompiledHeaderCPPFile )
                    {
                        continue;
                    }
                #endif

                // create the object that will compile the above file
                if ( CreateDynamicObjectNode( nodeGraph, n->GetName(), dln->GetPath() ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }
        }
        else if ( dep.GetNode()->GetType() == Node::UNITY_NODE )
        {
            // get the dir list from the unity node
            const UnityNode * un = dep.GetNode()->CastTo< UnityNode >();

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
                if ( CreateDynamicObjectNode( nodeGraph, n->GetName(), AString::GetEmpty(), true ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }

            // files from unity to build individually
            for ( const UnityIsolatedFile & isolatedFile : un->GetIsolatedFileNames() )
            {
                Node * n = nodeGraph.FindNode( isolatedFile.GetFileName() );
                if ( n == nullptr )
                {
                    n = nodeGraph.CreateFileNode( isolatedFile.GetFileName() );
                }
                else if ( n->IsAFile() == false )
                {
                    FLOG_ERROR( "Library() Isolated '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                    return false;
                }

                // create the object that will compile the above file
                if ( CreateDynamicObjectNode( nodeGraph, n->GetName(), isolatedFile.GetDirListOriginPath(), false, true ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }
        }
        else if ( dep.GetNode()->GetType() == Node::OBJECT_LIST_NODE )
        {
            // get the list of files from the ObjectListNode
            const ObjectListNode * objListNode = dep.GetNode()->CastTo< ObjectListNode >();
            Array< AString > objListFiles;
            objListNode->GetInputFiles( objListFiles );

            // iterate all the files in the object list
            for ( const AString & file : objListFiles )
            {
                const Node * n = nodeGraph.FindNode( file );
                if ( n == nullptr )
                {
                    FLOG_ERROR( "ObjectListNode: Missing Node '%s'", file.Get() );
                    return false;
                }
                else if ( n->IsAFile() == false )
                {
                    FLOG_ERROR( "ObjectListNode: '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                    return false;
                }

                // create the object that will compile the above file
                if ( CreateDynamicObjectNode( nodeGraph, n->GetName(), AString::GetEmpty() ) == false )
                {
                    return false; // CreateDynamicObjectNode will have emitted error
                }
            }
        }
        else if ( dep.GetNode()->IsAFile() )
        {
            // a single file, create the object that will compile it
            if ( CreateDynamicObjectNode( nodeGraph, dep.GetNode()->GetName(), AString::GetEmpty() ) == false )
            {
                return false; // CreateDynamicObjectNode will have emitted error
            }
        }
        else
        {
            ASSERT( false ); // unexpected node type
        }
    }

    // Depend on objects for loose files
    for ( const AString & file : m_CompilerInputFiles )
    {
        if ( CreateDynamicObjectNode( nodeGraph, file, AString::GetEmpty() ) == false )
        {
            return false; // CreateDynamicObjectNode will have emitted error
        }
    }

    // If we have a precompiled header, add that to our dynamic deps so that
    // any symbols in the PCH's .obj are also linked, when either:
    // a) we are a static library
    // b) a DLL or executable links our .obj files
    if ( m_PrecompiledHeaderName.IsEmpty() == false )
    {
        Node * node = nodeGraph.FindNode( m_PrecompiledHeaderName );
        ASSERT( node ); // Should always exist if we get here
        m_DynamicDependencies.EmplaceBack( node );
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
    if ( ( m_DynamicDependencies.GetSize() == 0 ) && ( m_CompilerInputAllowNoFiles == false ) )
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
/*virtual*/ Node::BuildResult ObjectListNode::DoBuild( Job * /*job*/ )
{
    // Generate stamp
    if ( m_DynamicDependencies.IsEmpty() )
    {
        m_Stamp = 1; // Non-zero
    }
    else
    {
        Array< uint64_t > stamps( m_DynamicDependencies.GetSize(), false );
        for ( const Dependency & dep : m_DynamicDependencies )
        {
            const ObjectNode * on = dep.GetNode()->CastTo< ObjectNode >();
            ASSERT( on->GetStamp() );
            stamps.Append( on->GetStamp() );
        }
        m_Stamp = xxHash::Calc64( &stamps[0], ( stamps.GetSize() * sizeof(uint64_t) ) );
    }

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
                if ( on->IsMSVC() || on->IsClangCl() )
                {
                    fullArgs += pre;
                    fullArgs += on->GetPCHObjectName();
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
            const ObjectListNode * oln = n->CastTo< ObjectListNode >();
            oln->GetInputFiles( fullArgs, pre, post, objectsInsteadOfLibs );
            continue;
        }

        // get objects used to create libs
        if ( ( n->GetType() == Node::LIBRARY_NODE ) && objectsInsteadOfLibs )
        {
            ASSERT( GetType() == Node::LIBRARY_NODE ); // should only be possible for a LibraryNode

            // insert all the objects in the object list
            const LibraryNode * ln = n->CastTo< LibraryNode >();
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

// GetObjectFileName
//------------------------------------------------------------------------------
void ObjectListNode::GetObjectFileName( const AString & fileName, const AString & baseDir, AString & objFile )
{
    // Transform src file to dst object path
    // get file name only (no path, no ext)
    const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
    lastSlash = lastSlash ? ( lastSlash + 1 ) : fileName.Get();
    const char * lastDot = m_CompilerOutputKeepBaseExtension ? fileName.GetEnd() : fileName.FindLast( '.' );
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
    objFile = m_CompilerOutputPath;
    objFile += subPath;
    objFile += m_CompilerOutputPrefix;
    objFile += fileNameOnly;
    objFile += GetObjExtension();
}

// CreateDynamicObjectNode
//------------------------------------------------------------------------------
bool ObjectListNode::CreateDynamicObjectNode( NodeGraph & nodeGraph,
                                              const AString & inputFileName,
                                              const AString & baseDir,
                                              bool isUnityNode,
                                              bool isIsolatedFromUnityNode )
{
    AStackString<> objFile;
    GetObjectFileName( inputFileName, baseDir, objFile );

    // Create an ObjectNode to compile the above file
    // and depend on that
    Node * on = nodeGraph.FindNode( objFile );
    if ( on == nullptr )
    {
        // Handle Unity modification of flags
        uint32_t flags = m_ObjFlags;
        if ( isUnityNode )
        {
            flags |= ObjectNode::FLAG_UNITY;
        }
        if ( isIsolatedFromUnityNode )
        {
            flags |= ObjectNode::FLAG_ISOLATED_FROM_UNITY;
        }

        const BFFToken * token = nullptr;
        ObjectNode * objectNode = CreateObjectNode( nodeGraph, token, nullptr, flags, m_ObjFlagsPreprocessor, m_CompilerOptions, m_CompilerOptionsDeoptimized, m_Preprocessor, m_PreprocessorOptions, objFile, inputFileName, AString::GetEmpty() );
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
        const ObjectNode * other = on->CastTo< ObjectNode >();

        // Check for conflicts
        const bool conflict = ( inputFileName != other->GetSourceFile()->GetName() ) ||
                              ( m_Name != other->GetOwnerObjectList() );
        if ( conflict )
        {
            FLOG_ERROR( "Conflicting objects found for: %s\n"
                        " Source A  : %s\n"
                        " ObjectList: %s\n"
                        "AND\n"
                        " Source B  : %s\n"
                        " ObjectList: %s\n",
                        objFile.Get(),
                        inputFileName.Get(), m_Name.Get(),
                        other->GetSourceFile()->GetName().Get(), other->GetOwnerObjectList().Get() );
            return false;
        }
    }
    m_DynamicDependencies.EmplaceBack( on );
    return true;
}

// CreateObjectNode
//------------------------------------------------------------------------------
ObjectNode * ObjectListNode::CreateObjectNode( NodeGraph & nodeGraph,
                                               const BFFToken * iter,
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
    node->m_PrecompiledHeader = m_PrecompiledHeaderName;
    node->m_Preprocessor = preprocessor;
    node->m_PreprocessorOptions = preprocessorOptions;
    node->m_Flags = flags;
    node->m_PreprocessorFlags = preprocessorFlags;
    node->m_OwnerObjectList = m_Name;

    if ( !node->Initialize( nodeGraph, iter, function ) )
    {
        // TODO:A We have a node in the graph which is in an invalid state
        return nullptr; // Initialize will have emitted an error
    }
    return node;
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

// EnumerateInputFiles
//------------------------------------------------------------------------------
void ObjectListNode::EnumerateInputFiles( void (*callback)( const AString & inputFile, const AString & baseDir, void * userData ), void * userData ) const
{
    // Statically specified files
    for ( const AString & file : m_CompilerInputFiles )
    {
        callback( file, AString::GetEmpty(), userData );
    }
    
    // Dynamically discovered files
    for ( size_t i = m_ObjectListInputStartIndex; i < m_ObjectListInputEndIndex; ++i )
    {
        const Node * node = m_StaticDependencies[ i ].GetNode();

        if ( node->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            const DirectoryListNode * dln = node->CastTo< DirectoryListNode >();

            const Array< FileIO::FileInfo > & files = dln->GetFiles();
            for ( const FileIO::FileInfo & fi : files )
            {
                callback( fi.m_Name, dln->GetPath(), userData );
            }
        }
        else if ( node->GetType() == Node::UNITY_NODE )
        {
            const UnityNode * un = node->CastTo< UnityNode >();

            un->EnumerateInputFiles( callback, userData );
        }
        else if ( node->IsAFile() )
        {
            callback( node->GetName(), AString::GetEmpty(), userData );
        }
        else
        {
            ASSERT( false ); // unexpected node type
        }
    }

}

//------------------------------------------------------------------------------
