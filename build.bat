@echo off
REM build.bat – configure and build SquareWaveSynth on Windows (VST3 only)
REM Usage:
REM   build.bat                          auto-fetch JUCE, Release
REM   build.bat --juce C:\path\to\JUCE  use local JUCE
REM   build.bat --debug                  Debug build

setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%
set BUILD_DIR=%SCRIPT_DIR%build_win
set BUILD_TYPE=Release
set JUCE_DIR=
set CLEAN=0

:parse_args
if "%~1"=="" goto :done_args
if "%~1"=="--juce"  ( set JUCE_DIR=%~2& shift & shift & goto :parse_args )
if "%~1"=="--debug" ( set BUILD_TYPE=Debug& shift & goto :parse_args )
if "%~1"=="--clean" ( set CLEAN=1& shift & goto :parse_args )
shift
goto :parse_args
:done_args

if %CLEAN%==1 (
    if exist "%BUILD_DIR%" (
        echo Cleaning build directory...
        rmdir /s /q "%BUILD_DIR%"
    )
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

set CMAKE_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if not "%JUCE_DIR%"=="" set CMAKE_ARGS=%CMAKE_ARGS% -DJUCE_SOURCE_DIR="%JUCE_DIR%"

echo Configuring (%BUILD_TYPE%)...
cmake "%SCRIPT_DIR%" %CMAKE_ARGS%
if errorlevel 1 ( echo CMake configure failed & exit /b 1 )

echo Building...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 ( echo Build failed & exit /b 1 )

echo.
echo Build complete!
echo.
echo To install VST3, copy:
echo   %BUILD_DIR%\SquareWaveSynth_artefacts\%BUILD_TYPE%\VST3\*.vst3
echo to:
echo   C:\Program Files\Common Files\VST3\
endlocal
