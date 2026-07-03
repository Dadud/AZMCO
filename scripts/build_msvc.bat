@echo off
:: Build AZMCO with MSVC 2022 / 2019
:: Output: %USERPROFILE%\projects\azmco-dev\build\<config>-<platform>\
::
:: Requires: Visual Studio 2019/2022 with C++ workload, MSVC v142 or v143 toolset
:: Optional: DirectX 8.0a SDK (only needed for legacy 32-bit D3D8 backend)
::
:: Usage: build_msvc.bat [Release|Debug] [Win32|x64]

setlocal

set "CFG=%~1"
if "%CFG%"=="" set "CFG=Release"
set "PLAT=%~2"
if "%PLAT%"=="" set "PLAT=Win32"

set "WORK=%USERPROFILE%\projects\azmco-dev"
set "UP=%WORK%\upstream"
set "OUT=%WORK%\build\%CFG%-%PLAT%"

echo AZMCO MSVC build
echo   upstream: %UP%
echo   output:   %OUT%
echo   config:   %CFG%
echo   platform: %PLAT%

:: Find MSBuild
set "MSBUILD="
for %%v in (2022 2019) do (
    for %%e in (Community Professional Enterprise BuildTools) do (
        if exist "C:\Program Files\Microsoft Visual Studio\%%v\%%e\MSBuild\Current\Bin\MSBuild.exe" (
            set "MSBUILD=C:\Program Files\Microsoft Visual Studio\%%v\%%e\MSBuild\Current\Bin\MSBuild.exe"
        )
    )
)
if "%MSBUILD%"=="" (
    echo ERROR: MSBuild not found. Install Visual Studio 2019/2022 with C++ workload.
    exit /b 1
)
echo   msbuild:  %MSBUILD%

if not exist "%UP%\AutoZone.sln" (
    echo ERROR: AutoZone.sln not found at %UP%. Re-clone upstream.
    exit /b 1
)

mkdir "%OUT%" 2>nul

"%MSBUILD%" "%UP%\AutoZone.sln" /t:Rebuild /p:Configuration=%CFG% /p:Platform=%PLAT% /p:OutDir=%OUT%\ /m:4

if errorlevel 1 (
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo BUILD SUCCEEDED.
echo Output in %OUT%
dir /b "%OUT%"
endlocal