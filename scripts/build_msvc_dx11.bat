@echo off
:: Build AZMCO DX11 backend (Phase 3) with MSVC 2022 Build Tools
:: Output: build\msvc-Release-x86\dx11\dx11.dll
::
:: Usage: scripts\build_msvc_dx11.bat [Release|Debug]
::
:: Requires:
::   - MSVC 2022 Build Tools (cl.exe 19.44+)
::   - Real DX8 SDK at upstream/SDK/DX80/ (for shared headers like IID_ID3D11Texture2D)
::   - d3d11.lib, dxgi.lib, d3dcompiler.lib (part of Windows SDK 10.0+)

setlocal

set "CFG=%~1"
if "%CFG%"=="" set "CFG=Release"

set "WORK=%USERPROFILE%\projects\azmco-dev"
set "UP=%WORK%\upstream\Source\R.DirectX.11.0.A"
set "OUT=%WORK%\build\msvc-%CFG%-x86\dx11"
set "TMP=%WORK%\build\msvc-%CFG%-x86\dx11"

if not exist "%TMP%" mkdir "%TMP%"

:: Find vcvars32
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
if not exist "%VCVARS%" (
    echo ERROR: MSVC 2022 vcvars32.bat not found at %VCVARS%
    exit /b 1
)

:: Single batch file that does compile + lib + link (avoids cmd-quoting issues)
set "SCRIPT=%TMP%\__build_dx11.bat"
(
    echo @echo off
    echo call "%VCVARS%" ^>nul
    echo cd /d "%UP%"
    echo cl.exe /c /nologo /O2 /EHsc /DDIRECT3D_VERSION=0x0b00 /D_WIN32_WINNT=0x0601 ^
    echo     /I"%WORK%\upstream\Source\AZX" ^
    echo     /I"%WORK%\upstream\SDK\DX80\Include" ^
    echo     /Fo"%TMP%\" ^
    echo     Module.cxx Main.cxx 2^>^&1
    echo if errorlevel 1 exit /b 1
    echo copy /Y "%UP%\Renderer.Module.MSVC.def" "%TMP%\Renderer.Module.fixed.def" ^>nul
    echo lib.exe /DEF:"%TMP%\Renderer.Module.fixed.def" /OUT:"%TMP%\dx11.lib" /MACHINE:X86 ^>nul
    echo if errorlevel 1 exit /b 1
    echo link.exe /DLL /OUT:"%TMP%\dx11.dll" /MACHINE:X86 /DEF:"%TMP%\Renderer.Module.fixed.def" ^
    echo     "%TMP%\Module.obj" "%TMP%\Main.obj" ^
    echo     d3d11.lib dxgi.lib d3dcompiler.lib user32.lib kernel32.lib gdi32.lib advapi32.lib 2^>^&1
    echo if errorlevel 1 exit /b 1
    echo echo BUILD_SUCCEEDED
) > "%SCRIPT%"

call "%SCRIPT%"
set "RC=%errorlevel%"
del "%SCRIPT%" 2>nul
if not "%RC%"=="0" (
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo dx11.dll built: %TMP%\dx11.dll
dir /b "%TMP%"
endlocal