echo :: Updating submodules
git submodule init
git submodule update --recursive

if %errorlevel% neq 0 (
    echo [ERROR] Failed to update submodules.
    pause
    exit /b %errorlevel%
)

:: Install vcpkg if not present
if not exist vcpkg (
    echo :: Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg
)
call .\vcpkg\bootstrap-vcpkg.bat
if %errorlevel% neq 0 (
    echo [ERROR] Failed to bootstrap vcpkg.
    pause
    exit /b %errorlevel%
)

:: Detect latest installed Visual Studio
set "VS_GEN="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationVersion`) do (
    set "VS_VER=%%i"
)
for /f "tokens=1 delims=." %%a in ("%VS_VER%") do (
    if %%a==17 set "VS_GEN=Visual Studio 17 2022"
    if %%a==16 set "VS_GEN=Visual Studio 16 2019"
    if %%a==15 set "VS_GEN=Visual Studio 15 2017"
)

if not defined VS_GEN (
    echo [ERROR] Could not detect Visual Studio generator.
    pause
    exit /b 1
)

echo :: Using generator: %VS_GEN%

:: Build directory
set "BUILD_DIR=IbInputSimulator\build"
rmdir /s /q "%BUILD_DIR%" 2>nul
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo :: Installing dependencies for x64-windows-static
..\..\vcpkg\vcpkg.exe install detours rapidjson --triplet=x64-windows-static
if %errorlevel% neq 0 (
    echo [ERROR] Failed to install dependencies.
    pause
    exit /b %errorlevel%
)

echo :: Configuring CMake project...
cmake .. -G "%VS_GEN%" -A x64 -DCMAKE_TOOLCHAIN_FILE="..\..\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b %errorlevel%
)

echo :: Building Debug
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo [ERROR] Debug build failed.
    pause
    exit /b %errorlevel%
)

echo :: Building Release
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Release build failed.
    pause
    exit /b %errorlevel%
)

cd ..\..

echo :: Copying build artifacts...
xcopy "%BUILD_DIR%\Simulator\Debug" "x64\Debug" /s /e /i /Y
xcopy "%BUILD_DIR%\Simulator\Release" "x64\Release" /s /e /i /Y

echo.
echo Build complete!
pause
