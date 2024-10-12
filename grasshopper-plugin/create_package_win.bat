@echo off

pushd %~dp0\YakPackage

del /s /q *.gha
del /s /q *.dll

xcopy ..\TapirGrasshopperPlugin\bin\Release\net48\*.gha . /s /d /y
xcopy ..\TapirGrasshopperPlugin\bin\Release\net48\*.dll . /s /d /y
xcopy ..\TapirGrasshopperPlugin\bin\Release\net7.0\*.gha net7.0 /I /s /d /y
xcopy ..\TapirGrasshopperPlugin\bin\Release\net7.0\*.dll net7.0 /I /s /d /y

call %1 build

popd
exit /b 0

:error
echo Build Failed with Error %errorlevel%.
popd
exit /b 1
