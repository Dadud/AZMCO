@echo off
:: Build AZMCO DX8 backend with MSVC 2022 Build Tools
:: Output: build\msvc-release\R.DirectX.8.0.A\dx8z.dll
::
:: Usage: scripts\build_msvc_dx8.bat [Release|Debug] [Win32|x64]

setlocal

set "CFG=%~1"
if "%CFG%"=="" set "CFG=Release"
set "PLAT=%~2"
if "%PLAT%"=="" set "PLAT=Win32"

set "WORK=%USERPROFILE%\projects\azmco-dev"
set "UP=%WORK%\upstream"
set "OUT=%WORK%\build\msvc-%CFG%-%PLAT%"

echo AZMCO DX8 build via MSVC
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
        if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%v\%%e\MSBuild\Current\Bin\MSBuild.exe" (
            set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\%%v\%%e\MSBuild\Current\Bin\MSBuild.exe"
        )
    )
)
if "%MSBUILD%"=="" (
    echo ERROR: MSBuild not found. Install Visual Studio 2022 Build Tools with C++ workload.
    exit /b 1
)
echo   msbuild:  %MSBUILD%

if not exist "%UP%\AutoZone.sln" (
    echo ERROR: AutoZone.sln not found at %UP%. Re-clone upstream.
    exit /b 1
)

mkdir "%OUT%" 2>nul

:: MSBuild needs absolute paths
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