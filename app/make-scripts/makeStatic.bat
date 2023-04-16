:Compiles static 64 bit single exe build of AFISync with MSVC
set QT_PATH=D:\Qt
set QT_STATIC_BINS=D:\qts\bin
set VS_DIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community

set CURRDIR=%cd%
cd %~dp0%..\..\..
set ROOT_DIR=%cd%
set RC_ZIP=D:\Dropbox\afisync2\AFISync-releases\AFISync_rc.zip
set SRC_DIR=%ROOT_DIR%\afi-sync\app
set SRC_BIN=%SRC_DIR%\bin
set BUILD_DIR=%ROOT_DIR%\build-AFISync-static
set RELEASE_DIR=%ROOT_DIR%\AFISync
set PERSONAL_DIR=%ROOT_DIR%\personal
set JSON_FILE=%PERSONAL_DIR%\settings\repositories.json
set PATH=%QT_STATIC_BINS%;%VS_DIR%\VC\Auxiliary\Build;%QT_PATH%\5.15.2\msvc2019_64\bin;%ROOT_DIR%;%systemroot%;%systemroot%\System32;%SRC_BIN%

goto create-zip

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
7za a %RC_ZIP% AFISync

:copy-personal
copy %BUILD_DIR%\release\AFISync.exe %PERSONAL_DIR%\
copy %BUILD_DIR%\release\AFISync.pdb %PERSONAL_DIR%\
copy %BUILD_DIR%\release\afisync_cmd.exe %PERSONAL_DIR%\

:end
cd %CURRDIR%
