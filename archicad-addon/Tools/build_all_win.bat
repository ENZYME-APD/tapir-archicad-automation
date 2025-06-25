pushd %~dp0
python download_and_unzip.py https://github.com/GRAPHISOFT/archicad-api-devkit/releases/download/25.3002/API.Development.Kit.WIN.25.3002.zip ..\Build\DevKits\AC25
python download_and_unzip.py https://github.com/GRAPHISOFT/archicad-api-devkit/releases/download/26.3000/API.Development.Kit.WIN.26.3000.zip ..\Build\DevKits\AC26
python download_and_unzip.py https://github.com/GRAPHISOFT/archicad-api-devkit/releases/download/27.3001/API.Development.Kit.WIN.27.3001.zip ..\Build\DevKits\AC27
python download_and_unzip.py https://github.com/GRAPHISOFT/archicad-api-devkit/releases/download/28.3001/API.Development.Kit.WIN.28.3001.zip ..\Build\DevKits\AC28

cmake -B ../Build/AC28 -G "Visual Studio 17 2022" -A "x64" -T "v142" -DAC_VERSION=28 -DAC_API_DEVKIT_DIR="..\Build\DevKits\AC28\Support" .. || goto :error
cmake --build ../Build/AC28 --config Debug || goto :error
cmake --build ../Build/AC28 --config RelWithDebInfo || goto :error

popd
exit /b 0

:error
echo Build Failed with Error %errorlevel%.
popd
exit /b 1
