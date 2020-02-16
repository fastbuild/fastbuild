// VSProjectExternalNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectExternalNode.h"


#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"


//  Visual Studio project Type GUID Extractor
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #pragma warning( push )
    #pragma warning (disable : 4530 4191)
    #include <VSProjLoaderInterface.h>
    #pragma warning( pop )
#endif

// Reflection
//------------------------------------------------------------------------------

REFLECT_STRUCT_BEGIN_BASE(VSExternalProjectConfig)
    REFLECT(m_Platform, "Platform", MetaNone())
    REFLECT(m_Config, "Config", MetaNone())
REFLECT_END(VSExternalProjectConfig)

REFLECT_NODE_BEGIN( VSProjectExternalNode, VSProjectBaseNode, MetaName( "ExternalProjectPath" ) + MetaFile() )
    REFLECT( m_ProjectGuid,     "ProjectGuid",     MetaOptional())
    REFLECT( m_projectTypeGuid, "ProjectTypeGuid", MetaOptional())
    REFLECT_ARRAY_OF_STRUCT(m_ProjectConfigs, "ProjectConfigs", VSExternalProjectConfig, MetaOptional())
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
/*virtual*/ bool VSProjectExternalNode::Initialize( NodeGraph& UNUSED(nodeGraph), const BFFToken* UNUSED(iter), const Function* UNUSED(function) )
{
    bool bHaveReflectedConfigs = !m_ProjectConfigs.IsEmpty();
    //
    // handle configs
    CopyConfigs();

    if (!bHaveReflectedConfigs)
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
/*virtual*/ Node::BuildResult VSProjectExternalNode::DoBuild( Job * UNUSED(job) )
{
    if (m_ProjectGuid.IsEmpty() || m_projectTypeGuid.IsEmpty() || m_ProjectConfigs.IsEmpty())
    {
        // analyze the external file
        if (!FileIO::FileExists(m_Name.Get()))
        {
            FLOG_ERROR("VSProjectExternalNode - External project file '%s' does not exist", m_Name.Get());
            return NODE_RESULT_FAILED;
        }
        else
        {
            // need to parse again?
            if (m_Stamp != FileIO::GetFileLastWriteTime(m_Name))
            {
                if (m_ProjectGuid.IsEmpty())
                {
                    // open the external project file
                    FileStream fs;
                    if (fs.Open(m_Name.Get(), FileStream::READ_ONLY) == false)
                    {
                        FLOG_ERROR("VSProjectExternalNode - Failed to open external project file '%s'", m_Name.Get());
                        return NODE_RESULT_FAILED;
                    }

                    // load project file into a string for parsing the project Guid
                    const size_t fileSize = (size_t)fs.GetFileSize();
                    AString extProjFileAsString;
                    extProjFileAsString.SetLength(uint32_t(fileSize));
                    if (fs.ReadBuffer(extProjFileAsString.Get(), fileSize) != fileSize)
                    {
                        FLOG_ERROR("VSProjectExternalNode - Failed to read external project file '%s'", m_Name.Get());
                        return NODE_RESULT_FAILED;
                    }

                    // parse Project GUID from string buffer
                    AString strPGStart(extProjFileAsString.FindI("<ProjectGuid>"));
                    AString strPGEnd(extProjFileAsString.FindI("</ProjectGuid>"));
                    size_t lenPGStart = AString::StrLen(strPGStart.Get());
                    size_t lenPGEnd = AString::StrLen(strPGEnd.Get());
                    size_t lenPG = lenPGStart - lenPGEnd;
                    m_ProjectGuid.SetLength(uint32_t(lenPG));
                    AString::Copy(extProjFileAsString.FindLastI("<ProjectGuid>"), m_ProjectGuid.Get(), lenPG);
                    m_ProjectGuid.Trim(uint32_t(AString::StrLen("<ProjectGuid>")), 0);

                    // some projects do not contain enclosing curled braces around the project GUID
                    if (!m_ProjectGuid.BeginsWith('{'))
                    {
                        AString tmp;
                        tmp.Append("{", 1);
                        tmp.Append(m_ProjectGuid.Get(), m_ProjectGuid.GetLength());
                        m_ProjectGuid = tmp;
                    }
                    if (!m_ProjectGuid.EndsWith('}'))
                    {
                        m_ProjectGuid.Append("}", 1);
                    }
                }
                // get ProjectTypeGUID from external Visual Studio project Type GUID Extractor module 'VSProjectExternal'
#if defined( __WINDOWS__ )
                if (m_projectTypeGuid.IsEmpty() || m_ProjectConfigs.IsEmpty())
                {
                    if (!VspteModuleWrapper::Instance()->IsLoaded())
                    {
                        // load the module if not already loaded
                        VspteModuleWrapper::Instance()->Load();
                    }
                    if (VspteModuleWrapper::Instance()->IsLoaded())
                    {
                        ExtractedProjData projData;
                        bool bSuccess = VspteModuleWrapper::Instance()->Vspte_GetProjData(
                            m_Name.Get(),
                            &projData);
                        if (bSuccess)
                        {
                            // copy project type Guid
                            if (m_projectTypeGuid.IsEmpty())
                            {
                                m_projectTypeGuid = projData._TypeGuid;
                            }

                            // copy config / platform tuples
                            if (m_ProjectConfigs.IsEmpty() && projData._numCfgPlatforms)
                            {
                                VSExternalProjectConfig ExtPlatCfgTuple;
                                m_ProjectConfigs.SetCapacity(projData._numCfgPlatforms);
                                for (unsigned int i = 0; i < projData._numCfgPlatforms; i++)
                                {
                                    ExtPlatCfgTuple.m_Config = projData._pConfigsPlatforms[i]._config;
                                    ExtPlatCfgTuple.m_Platform = projData._pConfigsPlatforms[i]._platform;
                                    m_ProjectConfigs.Append(ExtPlatCfgTuple);
                                }
                            }

                            //
                            VspteModuleWrapper::Instance()->Vspte_DeallocateProjDataCfgArray(&projData);
                        }
                        else
                        {
                            VspteModuleWrapper::Instance()->Vspte_DeallocateProjDataCfgArray(&projData);
                            FLOG_ERROR("VSProjectExternalNode - Failed retrieving type Guid and / or config|platform pairs for external project '%s', please check the output or the log of the 'VSProjectExternal' module! Maybe explicitely providing the project data is required.", m_Name.Get());
                            return NODE_RESULT_FAILED;
                        }
                    }
                    else
                    {
                        FLOG_ERROR("VSProjectExternalNode - Failed to load the external VSProjTypeExtractor module, please consult the 'VSProjectExternal' documentation! Maybe explicitely providing the project data is required.");
                        return NODE_RESULT_FAILED;
                    }
                }
#endif
                //
                // handle configs
                CopyConfigs();
            }
        }
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

void VSProjectExternalNode::CopyConfigs()
{
    //
    // add missing default configs
    Array<VSExternalProjectConfig> defaultConfigs;
    VSExternalProjectConfig cfgDefault;
    defaultConfigs.SetCapacity(4);
    cfgDefault.m_Platform = "Win32";
    cfgDefault.m_Config = "Debug";
    defaultConfigs.Append(cfgDefault);
    cfgDefault.m_Config = "Release";
    defaultConfigs.Append(cfgDefault);
    cfgDefault.m_Platform = "x64";
    defaultConfigs.Append(cfgDefault);
    cfgDefault.m_Config = "Debug";
    defaultConfigs.Append(cfgDefault);
    for (const VSExternalProjectConfig& defaultConfig : defaultConfigs)
    {
        bool bFound = false;
        for (const VSExternalProjectConfig& existingConfig : m_ProjectConfigs)
        {
            if (defaultConfig.m_Config == existingConfig.m_Config &&
                defaultConfig.m_Platform == existingConfig.m_Platform)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            m_ProjectConfigs.Append(defaultConfig);
        }
    }

    // copy platform config tuples array to base class
    if (!m_ProjectConfigs.IsEmpty())
    {
        m_ProjectPlatformConfigTuples.Clear();
        VSProjectPlatformConfigTuple PlatCfgTuple;
        m_ProjectPlatformConfigTuples.SetCapacity(m_ProjectConfigs.GetSize());
        for (const VSExternalProjectConfig& config : m_ProjectConfigs)
        {
            PlatCfgTuple.m_Config = config.m_Config;
            PlatCfgTuple.m_Platform = config.m_Platform;
            m_ProjectPlatformConfigTuples.Append(PlatCfgTuple);
        }
    }
}

//------------------------------------------------------------------------------
