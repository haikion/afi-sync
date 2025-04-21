:Compiles static 64 bit single exe build of AFISync with MSVC
set QT_STATIC_BINS=E:\qts\bin
set VS_DIR=E:\Program Files\Microsoft Visual Studio\2022\Community

set CURRDIR=%cd%
cd %~dp0%..\..\..
set ROOT_DIR=%cd%
set RC_ZIP=E:\afisync2\AFISync-releases\AFISync_rc.zip
set SRC_DIR=%ROOT_DIR%\afi-sync\app
set SRC_BIN=%SRC_DIR%\bin
set BUILD_DIR=%ROOT_DIR%\build-AFISync-static
set RELEASE_DIR=%ROOT_DIR%\AFISync
set PERSONAL_DIR=%ROOT_DIR%\personal
set JSON_FILE=%PERSONAL_DIR%\settings\repositories.json
set OPENSSL_ROOT_DIR=E:\afisync2\openssl-3.4.1
set PATH=%QT_STATIC_BINS%;%VS_DIR%\VC\Auxiliary\Build;%ROOT_DIR%;%systemroot%;%systemroot%\System32;%SRC_BIN%;C:\Windows\System32\OpenSSH;E:\cygwin64\bin;E:\unison\bin

rmdir /S %BUILD_DIR%
rmdir /S %RELEASE_DIR%

mkdir %BUILD_DIR%

:compile
call vcvarsall.bat x86_amd64 || exit /b 1

cd %BUILD_DIR%
cmake %SRC_DIR% -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" || exit /b 1
nmake || exit /b 1

:init-release
mkdir %RELEASE_DIR%

:copy-bins
cd %RELEASE_DIR%
copy %BUILD_DIR%\AFISync.exe
copy %BUILD_DIR%\afisync_cmd.exe
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
copy %BUILD_DIR%\AFISync.exe %PERSONAL_DIR%\
copy %BUILD_DIR%\afisync_cmd.exe %PERSONAL_DIR%\

:end
cd %CURRDIR%
