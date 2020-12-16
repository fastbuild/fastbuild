/*
    Implements loading / unloading VSProjTypeExtractor.dll dynamically, in order to avoid linking against the .lib
*/

#include <Windows.h>

//
// still need to define these ourselves, as an exact copy of the  respective #defines and structs from VSProjTypeExtractor.h:
//
#define VSPROJ_TYPEEXTRACT_MAXGUID_LENGTH     39
#define VSPROJ_MAXSTRING_LENGTH              120

// configuration / platform pair
typedef struct
{
	char _config[VSPROJ_MAXSTRING_LENGTH];
	char _platform[VSPROJ_MAXSTRING_LENGTH];
} ExtractedCfgPlatform;

// extracted project data containing type GUID and array of found configuration / platform pairs
typedef struct
{
	char _TypeGuid[VSPROJ_TYPEEXTRACT_MAXGUID_LENGTH];
	ExtractedCfgPlatform* _pConfigsPlatforms;
	unsigned int _numCfgPlatforms;
} ExtractedProjData;

// prototypes of exported functions
typedef bool  (__stdcall *Type_GetProjData)(const char* projPath, ExtractedProjData* pProjData);
typedef void* (__stdcall *Type_CleanUp)(void);
typedef void* (__stdcall *Type_DeallocateProjDataCfgArray)(ExtractedProjData* pProjData);


//
// wrapper around the external VSProjTypeExtractor.dll functionality
class VspteModuleWrapper
{
private:
	VspteModuleWrapper()
	{
		Load();
	}
	~VspteModuleWrapper()
	{
		if (_hVSProjTypeExtractor)
		{
			Vspte_CleanUp();
			::FreeLibrary(_hVSProjTypeExtractor);
			//
			_hVSProjTypeExtractor = nullptr;
			_Vspte_GetProjData = nullptr;
			_Vspte_CleanUp = nullptr;
			_Vspte_DeallocateProjDataCfgArray = nullptr;
		}
	}
	void Load()
	{
		if (!_hVSProjTypeExtractor)
		{
			_hVSProjTypeExtractor = ::LoadLibrary("VSProjTypeExtractor");
			if (_hVSProjTypeExtractor)
			{
				_Vspte_GetProjData = reinterpret_cast<Type_GetProjData>(::GetProcAddress(_hVSProjTypeExtractor, "Vspte_GetProjData"));
				_Vspte_CleanUp = reinterpret_cast<Type_CleanUp>(::GetProcAddress(_hVSProjTypeExtractor, "Vspte_CleanUp"));
				_Vspte_DeallocateProjDataCfgArray = reinterpret_cast<Type_DeallocateProjDataCfgArray>(::GetProcAddress(_hVSProjTypeExtractor, "Vspte_DeallocateProjDataCfgArray"));
			}
		}
	}
	void Vspte_CleanUp()
	{
		if (_Vspte_CleanUp)
		{
			_Vspte_CleanUp();
		}
	}

	Type_GetProjData _Vspte_GetProjData = nullptr;
	Type_CleanUp _Vspte_CleanUp = nullptr;
	Type_DeallocateProjDataCfgArray _Vspte_DeallocateProjDataCfgArray = nullptr;
	HMODULE _hVSProjTypeExtractor = nullptr;


public:
	/** @brief  Access to singleton

	*/
	static VspteModuleWrapper* Instance()
	{
		static VspteModuleWrapper s_Instance;
		return &s_Instance;
	}

	/** @brief  Queries if VSProjTypeExtractor.dll was successfully loaded

		Helpful for avoiding to call Vspte_GetProjData, Vspte_DeallocateProjDataCfgArray or Vspte_CleanUp without effect
	*/
	bool IsLoaded()
	{
		return _hVSProjTypeExtractor != nullptr && _Vspte_GetProjData != nullptr && _Vspte_CleanUp != nullptr && _Vspte_DeallocateProjDataCfgArray != nullptr;
	}

	/** @brief  Retrieves basic project data from an existing project

		The project data is extracted by silently automating the loading of the project in a volatile solution of a new,
		hidden Visual Studio instance.

		@param[in] projPath path to visual studio project file
		@param[in,out] pProjData for receiving the project type GUID and existing configurations
	*/
	bool Vspte_GetProjData(const char* projPath, ExtractedProjData* pProjData)
	{
		if (_Vspte_GetProjData)
		{
			return _Vspte_GetProjData(projPath, pProjData);
		}
		else
		{
			return false;
		}
	}

	/** @brief  Deallocates the configurations / platforms array of an ExtractedProjData instance already used in a call to Vspte_GetProjData

		After a call to Vspte_GetProjData and copying the data you're interested in from the ExtractedProjData object, you should call this
		in order to deallocate the configurations / platforms array with the correct runtime
	*/
	void Vspte_DeallocateProjDataCfgArray(ExtractedProjData* pProjData)
	{
		if (_Vspte_DeallocateProjDataCfgArray)
		{
			_Vspte_DeallocateProjDataCfgArray(pProjData);
		}
	}
};
