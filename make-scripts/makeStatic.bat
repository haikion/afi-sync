:Compiles static 64 bit single exe build of AFISync with MSVC-14
set QT_PATH=C:\Qt
set QT_STATIC_BINS=D:\qts\5.8\bin
set VS_DIR=D:\Microsoft Visual Studio 14.0

set CURRDIR=%cd%
cd %~dp0%..\..
set ROOT_DIR=%cd%
set RC_ZIP=%ROOT_DIR%\AFISync-releases\AFISync_rc.zip
set SRC_DIR=%ROOT_DIR%\afi-sync
set BUILD_DIR=%ROOT_DIR%\build-AFISync-static
set RELEASE_DIR=%ROOT_DIR%\AFISync
set PERSONAL_DIR=%ROOT_DIR%\personal
set JSON_FILE=%PERSONAL_DIR%\settings\repositories.json
set PATH=%QT_STATIC_BINS%;%VS_DIR%\VC;%VS_DIR%\Common7\Tools;%QT_PATH%\5.8\msvc2015_64\bin;%ROOT_DIR%;%systemroot%;%systemroot%\System32

rmdir /S %BUILD_DIR%
rmdir /S %RELEASE_DIR%

mkdir %BUILD_DIR%

:compile
call vcvarsall.bat x86_amd64

cd %BUILD_DIR%
qmake %SRC_DIR% "CONFIG += console"
nmake
move release\AFISync.exe release\afisync_cmd.exe
nmake clean
qmake %SRC_DIR%
nmake

:init-release
mkdir %RELEASE_DIR%

:copy-bins
cd %RELEASE_DIR%
copy %BUILD_DIR%\release\afisync_cmd.exe
copy %BUILD_DIR%\release\AFISync.exe
mkdir bin
xcopy /E %SRC_DIR%\bin bin
copy %SRC_DIR%\afisync_header.png

:make-settings
mkdir settings
copy %JSON_FILE% settings\

:create-zip
cd %ROOT_DIR%
del %RC_ZIP%
zip -r %RC_ZIP% AFISync

:copy-personal
copy %BUILD_DIR%\release\AFISync.exe %PERSONAL_DIR%\
copy %BUILD_DIR%\release\AFISync.pdb %PERSONAL_DIR%\
copy %BUILD_DIR%\release\afisync_cmd.exe %PERSONAL_DIR%\
goto end

:end
cd %CURRDIR%
