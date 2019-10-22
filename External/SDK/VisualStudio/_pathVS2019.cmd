@echo off
set "DirVswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"

:: detect VS2019
for /f "usebackq tokens=1* delims=: " %%i in (`"%DirVswhere%\vswhere.exe" -nologo -version ^[16.0^,17.0^)`) do (
	if /i "%%i"=="installationPath" (
		set "my_VS_BasePath=%%j"
		set "my_VSCOMNTOOLS=%%j\Common7\Tools\"
	)
	if /i "%%i"=="productPath" (
		set "my_DevEnv=%%j"
	)
)
@set "DirVswhere="

:: detect Windows 10 SDK
@set VSCMD_ARG_HOST_ARCH=x64
@set VSCMD_ARG_TGT_ARCH=x64
@set "winSDKScript=%my_VSCOMNTOOLS%\vsdevcmd\core\winsdk.bat"
if exist "%winSDKScript%" (
	@echo Executing "%winSDKScript%" ...
	@call "%winSDKScript%"
) else (
	@echo ERROR: "%winSDKScript%" NOT FOUND, Visual Studio installation changed/damaged ^?
	exit /B 1
)
@set "my_Win_SDKBasePath=%WindowsSdkDir:~0,-1%"
@set "my_Win_SDKVersion=%WindowsSDKVersion:~0,-1%"

@echo ################################################################################
@echo Detected / overridable paths / versions:
@echo "my_VS_BasePath=%my_VS_BasePath%"
@echo "my_VS_Version=%my_VS_Version%"
@echo "my_DevEnv=%my_DevEnv%"
@echo "my_Win_SDKBasePath=%my_Win_SDKBasePath%"
@echo "my_Win_SDKVersion=%my_Win_SDKVersion%"
@echo ################################################################################
