@echo off
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install CMake and enable "Add CMake to PATH".
  exit /b 1
)
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build-win --config Release --parallel
if errorlevel 1 exit /b 1
echo.
echo YARIFILTER build complete:
echo build-win\YARIFILTER_artefacts\Release\VST3\YARIFILTER.vst3
