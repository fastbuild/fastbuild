// VSProjectExternalNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectExternalNode.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"

//  Visual Studio project Type GUID Extractor
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    PRAGMA_DISABLE_PUSH_MSVC( 4191 ) // C4191: 'reinterpret_cast': unsafe conversion from 'FARPROC' to 'Type_CleanUp'
    PRAGMA_DISABLE_PUSH_MSVC( 4530 ) // C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
    #include <VSProjLoaderInterface.h>
    PRAGMA_DISABLE_POP_MSVC // 4530
    PRAGMA_DISABLE_POP_MSVC // 4191
#endif

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( VSExternalProjectConfig )
    REFLECT( m_Platform, "Platform", MetaNone() )
    REFLECT( m_Config, "Config", MetaNone() )
REFLECT_END( VSExternalProjectConfig )

REFLECT_NODE_BEGIN( VSProjectExternalNode, VSProjectBaseNode, MetaName( "ExternalProjectPath" ) + MetaFile() )
    REFLECT( m_ProjectTypeGuid, "ProjectTypeGuid", MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_ProjectConfigs, "ProjectConfigs", VSExternalProjectConfig, MetaOptional() )
REFLECT_END( VSProjectExternalNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectExternalNode::VSProjectExternalNode()
    : VSProjectBaseNode()
{
    m_Type = Node::VSPROJEXTERNAL_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool VSProjectExternalNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*iter*/, const Function * /*function*/ )
{
    const bool hasReflectedConfigs = !m_ProjectConfigs.IsEmpty();

    // handle configs
    CopyConfigs();

    if ( !hasReflectedConfigs )
    {
        m_ProjectConfigs.Clear();
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectExternalNode::~VSProjectExternalNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult VSProjectExternalNode::DoBuild( Job * /*job*/ )
{
    if ( m_ProjectGuid.IsEmpty() || m_ProjectTypeGuid.IsEmpty() || m_ProjectConfigs.IsEmpty() )
    {
        const uint64_t timeStamp = FileIO::GetFileLastWriteTime( m_Name );

        // Report errors for missing files
        if ( timeStamp == 0 )
        {
            FLOG_ERROR( "VSProjectExternalNode - External project file '%s' does not exist", m_Name.Get() );
            return NODE_RESULT_FAILED;
        }

        // Did the file change?
        if ( m_Stamp != timeStamp )
        {
            // Extract the ProjectGuid if not manually provided
            if ( m_ProjectGuid.IsEmpty() )
            {
                // open the external project file
                FileStream fs;
                if ( fs.Open( m_Name.Get(), FileStream::READ_ONLY ) == false )
                {
                    FLOG_ERROR( "VSProjectExternalNode - Failed to open external project file '%s'", m_Name.Get() );
                    return NODE_RESULT_FAILED;
                }

                // load project file into a string for parsing the project Guid
                const size_t fileSize = (size_t)fs.GetFileSize();
                AString extProjFileAsString;
                extProjFileAsString.SetLength( (uint32_t)fileSize );
                if ( fs.ReadBuffer( extProjFileAsString.Get(), fileSize ) != fileSize )
                {
                    FLOG_ERROR( "VSProjectExternalNode - Failed to read external project file '%s'", m_Name.Get() );
                    return NODE_RESULT_FAILED;
                }

                // parse Project GUID from string buffer
                const char * strPGStart = extProjFileAsString.FindI( "<ProjectGuid>" );
                const char * strPGEnd = extProjFileAsString.FindI( "</ProjectGuid>" );
                if ( ( strPGStart == nullptr ) || ( strPGEnd == nullptr ) )
                {
                    FLOG_ERROR( "VSProjectExternalNode - Failed to extract <ProjectGuid> project file '%s'", m_Name.Get() );
                    return NODE_RESULT_FAILED;
                }
                m_ProjectGuid.Assign( strPGStart + 13, strPGEnd ); // +13 to trim <ProjectGuid>

                // some projects do not contain enclosing curled braces around the project GUID
                if ( !m_ProjectGuid.BeginsWith( '{' ) )
                {
                    AStackString<> tmp( "{" );
                    tmp += m_ProjectGuid;
                    m_ProjectGuid = tmp;
                }
                if ( !m_ProjectGuid.EndsWith( '}' ) )
                {
                    m_ProjectGuid += '}';
                }
            }

            // get ProjectTypeGUID from external Visual Studio project Type GUID Extractor module 'VSProjectExternal'
            #if defined( __WINDOWS__ )
                if ( m_ProjectTypeGuid.IsEmpty() || m_ProjectConfigs.IsEmpty() )
                {
                    // the wrapper singleton will load the DLL only at the first invocation of the constructor
                    if ( VspteModuleWrapper::Instance()->IsLoaded() )
                    {
                        ExtractedProjData projData;
                        if ( VspteModuleWrapper::Instance()->Vspte_GetProjData( m_Name.Get(), &projData ))
                        {
                            // copy project type Guid
                            if ( m_ProjectTypeGuid.IsEmpty() )
                            {
                                m_ProjectTypeGuid = projData._TypeGuid;
                            }

                            // copy config / platform tuples
                            if ( m_ProjectConfigs.IsEmpty() && projData._numCfgPlatforms )
                            {
                                VSExternalProjectConfig ExtPlatCfgTuple;
                                m_ProjectConfigs.SetCapacity( projData._numCfgPlatforms );
                                for ( uint32_t i = 0; i < projData._numCfgPlatforms; i++ )
                                {
                                    ExtPlatCfgTuple.m_Config = projData._pConfigsPlatforms[ i ]._config;
                                    ExtPlatCfgTuple.m_Platform = projData._pConfigsPlatforms[ i ]._platform;
                                    m_ProjectConfigs.Append( ExtPlatCfgTuple );
                                }
                            }

                            //
                            VspteModuleWrapper::Instance()->Vspte_DeallocateProjDataCfgArray( &projData );
                        }
                        else
                        {
                            VspteModuleWrapper::Instance()->Vspte_DeallocateProjDataCfgArray( &projData );
                            FLOG_ERROR( "VSProjectExternalNode - Failed retrieving type Guid and / or config|platform pairs for external project '%s', please check the output or the log of the 'VSProjectExternal' module! Maybe explicitely providing the project data is required.", m_Name.Get() );
                            return NODE_RESULT_FAILED;
                        }
                    }
                    else
                    {
                        FLOG_ERROR( "VSProjectExternalNode - Failed to load the external VSProjTypeExtractor module, please consult the 'VSProjectExternal' documentation! Maybe explicitely providing the project data is required." );
                        return NODE_RESULT_FAILED;
                    }
                }
            #endif

            // handle configs
            CopyConfigs();
        }
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// CopyConfigs
//------------------------------------------------------------------------------
void VSProjectExternalNode::CopyConfigs()
{
    //
    // add missing default configs
    Array<VSExternalProjectConfig> defaultConfigs;
    VSExternalProjectConfig cfgDefault;
    defaultConfigs.SetCapacity( 4 );
    cfgDefault.m_Platform = "Win32";
    cfgDefault.m_Config = "Debug";
    defaultConfigs.Append( cfgDefault );
    cfgDefault.m_Config = "Release";
    defaultConfigs.Append( cfgDefault );
    cfgDefault.m_Platform = "x64";
    defaultConfigs.Append( cfgDefault );
    cfgDefault.m_Config = "Debug";
    defaultConfigs.Append( cfgDefault );
    for ( const VSExternalProjectConfig & defaultConfig : defaultConfigs )
    {
        bool found = false;
        for ( const VSExternalProjectConfig & existingConfig : m_ProjectConfigs )
        {
            if ( ( defaultConfig.m_Config == existingConfig.m_Config ) &&
                 ( defaultConfig.m_Platform == existingConfig.m_Platform ) )
            {
                found = true;
                break;
            }
        }
        if ( !found )
        {
            m_ProjectConfigs.Append( defaultConfig );
        }
    }

    // copy platform config tuples array to base class
    if ( !m_ProjectConfigs.IsEmpty() )
    {
        m_ProjectPlatformConfigTuples.Clear();
        VSProjectPlatformConfigTuple platCfgTuple;
        m_ProjectPlatformConfigTuples.SetCapacity( m_ProjectConfigs.GetSize() );
        for ( const VSExternalProjectConfig & config : m_ProjectConfigs )
        {
            platCfgTuple.m_Config = config.m_Config;
            platCfgTuple.m_Platform = config.m_Platform;
            m_ProjectPlatformConfigTuples.Append( platCfgTuple );
        }
    }
}

// GetProjectTypeGuid
//------------------------------------------------------------------------------
/*virtual*/ const AString & VSProjectExternalNode::GetProjectTypeGuid() const 
{
    return m_ProjectTypeGuid;
}

//------------------------------------------------------------------------------
