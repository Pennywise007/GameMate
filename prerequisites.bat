:: Updating submodules
git submodule init
git submodule update --recursive
 
:: Installing IbInputSimulator dependencies

:: Installing vcpkg
git clone https://github.com/microsoft/vcpkg
call .\vcpkg\bootstrap-vcpkg.bat

set "InputSimulatorBuildDir=IbInputSimulator\build"

:: Building x86
.\vcpkg\vcpkg.exe install detours rapidjson --triplet=x86-windows-static

rmdir /s /q "%InputSimulatorBuildDir%"
mkdir %InputSimulatorBuildDir%
pushd %InputSimulatorBuildDir%
cmake .. -DCMAKE_TOOLCHAIN_FILE="..\..\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x86-windows-static -DCMAKE_GENERATOR_PLATFORM=WIN32
cmake --build . --config Debug
cmake --build . --config Release
popd

xcopy "%InputSimulatorBuildDir%\Simulator\Debug" "Debug" /s /e /i /Y
xcopy "%InputSimulatorBuildDir%\Simulator\Release" "Release" /s /e /i /Y

:: Building x64
.\vcpkg\vcpkg.exe install detours rapidjson --triplet=x64-windows-static

rmdir /s /q "%InputSimulatorBuildDir%"
mkdir %InputSimulatorBuildDir%
pushd %InputSimulatorBuildDir%
cmake .. -DCMAKE_TOOLCHAIN_FILE="..\..\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build . --config Debug
cmake --build . --config Release
popd

xcopy "%InputSimulatorBuildDir%\Simulator\Debug" "x64\Debug" /s /e /i /Y
xcopy "%InputSimulatorBuildDir%\Simulator\Release" "x64\Release" /s /e /i /Y
