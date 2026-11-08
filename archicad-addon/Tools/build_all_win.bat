pushd %~dp0
rem Every Archicad version in ..\devkits.json, as "<version> <DevKit URL>" lines.
for /f "tokens=1,2" %%a in ('python ..\..\tools\devkits.py lines WIN') do (
    call :build %%a %%b || goto :error
)

popd
exit /b 0

:build
python download_and_unzip.py %2 ..\Build\DevKits\AC%1 || exit /b 1
set toolset=v143
if %1 LSS 29 set toolset=v142
cmake -B ../Build/AC%1 -G "Visual Studio 18 2026" -A "x64" -T "%toolset%" -DAC_VERSION=%1 -DAC_API_DEVKIT_DIR="..\Build\DevKits\AC%1\Support" .. || exit /b 1
cmake --build ../Build/AC%1 --config Debug || exit /b 1
cmake --build ../Build/AC%1 --config RelWithDebInfo || exit /b 1
exit /b 0

:error
echo Build Failed with Error %errorlevel%.
popd
exit /b 1
