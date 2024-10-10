@echo off

pushd %~dp0\..\YakPackage

rmdir /s /q net48
rmdir /s /q net7.0

mkdir net48
mkdir net7.0

xcopy ..\TapirGrasshopperPlugin\bin\Release\net48\* net48 /s /d /y
xcopy ..\TapirGrasshopperPlugin\bin\Release\net7.0\* net7.0 /s /d /y

pushd 

popd
exit /b 0

:error
echo Build Failed with Error %errorlevel%.
popd
exit /b 1
