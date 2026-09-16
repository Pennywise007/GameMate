@echo off

set "BUILD_DIR=IbInputSimulator\build"

echo :: Updating submodules
git submodule update --init --recursive
if errorlevel 1 exit /b 1

echo :: Installing vcpkg
if not exist vcpkg (
    git clone https://github.com/microsoft/vcpkg
    if errorlevel 1 exit /b 1
)

if not exist vcpkg\vcpkg.exe (
    call vcpkg\bootstrap-vcpkg.bat
    if errorlevel 1 exit /b 1
)

echo :: Installing dependencies
vcpkg\vcpkg.exe install detours rapidjson --triplet=x64-windows
if errorlevel 1 exit /b 1

echo :: Preparing build directory
rmdir /s /q "%BUILD_DIR%" 2>nul

echo :: Detecting Visual Studio

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found.
    pause
    exit /b 1
)

set "VS_YEAR="

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do (
    set "VS_YEAR=%%i"
)

if not defined VS_YEAR (
    echo [ERROR] Visual Studio with C++ tools was not found.
    pause
    exit /b 1
)

echo :: Visual Studio: %VS_YEAR%

if "%VS_YEAR%"=="2022" set "VS_GEN=Visual Studio 17 2022"
if "%VS_YEAR%"=="2019" set "VS_GEN=Visual Studio 16 2019"
if "%VS_YEAR%"=="2017" set "VS_GEN=Visual Studio 15 2017"
if "%VS_YEAR%"=="2026" set "VS_GEN=Visual Studio 18 2026"

if not defined VS_GEN (
    echo [ERROR] Unsupported Visual Studio version: %VS_YEAR%
    pause
    exit /b 1
)

echo :: Using generator: %VS_GEN%

echo :: Configuring CMake

cmake ^
    -S IbInputSimulator ^
    -B "%BUILD_DIR%" ^
    -G "%VS_GEN%" ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_SHARED_LIBS=OFF

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo :: Building Debug
cmake --build "%BUILD_DIR%" --config Debug
if errorlevel 1 exit /b 1

echo :: Building Release
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo :: Copying build artifacts

mkdir "x64\Debug" 2>nul
mkdir "x64\Release" 2>nul

xcopy "%BUILD_DIR%\Simulator\Debug\*" "x64\Debug\" /s /e /i /Y
xcopy "%BUILD_DIR%\Simulator\Release\*" "x64\Release\" /s /e /i /Y

xcopy "vcpkg\installed\x64-windows\debug\lib\*" "x64\Debug\" /s /e /i /Y
xcopy "vcpkg\installed\x64-windows\lib\*" "x64\Release\" /s /e /i /Y

echo.
echo ========================================
echo Build complete!
echo ========================================
pause