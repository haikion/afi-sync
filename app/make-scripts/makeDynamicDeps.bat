:Compiles Boost and libtorrent. Copies libs to lib folder.
set OLD_DIR=%cd%
cd %~dp0
cd ..\..\..\
set VS_DIR=E:\Program Files\Microsoft Visual Studio\2022\Community
set BIN_PATH=%cd%\afi-sync\app\bin
set SRC_PATH=%cd%\src
set LIB_PATH=%cd%\lib
set BOOST_BUILD_PATH=%SRC_PATH%\boost_1_87_0
set LIBTORRENT_BUILD_PATH=%SRC_PATH%\libtorrent-rasterbar-2.0.11
set PARAMS=--with-system --with-date_time --with-atomic --with-random --with-log --with-filesystem --with-thread architecture=x86 address-model=64 toolset=msvc
set MSVC_PARAMS_DYNAMIC=%PARAMS% link=shared runtime-link=shared runtime-debugging=on variant=debug
set PARAMS_LIBTORRENT=crypto=built-in dht=off encryption=off i2p=off
set MSVC_PARAMS_DYNAMIC_LIBTORRENT=%MSVC_PARAMS_DYNAMIC% %PARAMS_LIBTORRENT%
set MSVC_PARAMS_STATIC=%PARAMS% link=static runtime-link=static variant=release
set MSVC_PARAMS_STATIC_LIBTORRENT=%MSVC_PARAMS_STATIC% %PARAMS_LIBTORRENT%
set PATH=%BOOST_BUILD_PATH%;%BIN_PATH%;%VS_DIR%\VC\Auxiliary\Build;%systemroot%;%systemroot%\System32;C:\Perl64\bin;C:\Windows\System32\OpenSSH;E:\cygwin64\bin;E:\unison\bin

call vcvarsall.bat x86_amd64 || exit /b 1
cd %SRC_PATH%

rmdir /s %LIBTORRENT_BUILD_PATH%
rmdir /s %BOOST_BUILD_PATH%
7za x %BOOST_BUILD_PATH%.7z
tar -xvzf %LIBTORRENT_BUILD_PATH%.tar.gz
mkdir ..\lib

:msvc-dynamic
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2 %MSVC_PARAMS_DYNAMIC%

:msvc-dynamic-libtorrent
cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS_DYNAMIC_LIBTORRENT%

:msvc-dynamic-install
copy %BOOST_BUILD_PATH%\stage\lib\* %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.3\debug\address-model-64\architecture-x86\crypto-built-in\cxxstd-14-iso\dht-off\encryption-off\i2p-off\threading-multi\torrent-rasterbar.dll %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.3\debug\address-model-64\architecture-x86\crypto-built-in\cxxstd-14-iso\dht-off\encryption-off\i2p-off\threading-multi\torrent.lib %LIB_PATH%

goto end

:msvc-static
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2  %MSVC_PARAMS_STATIC%

:msvc-static-libtorrent
cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS_STATIC_LIBTORRENT%

:msvc-static-install
copy %BOOST_BUILD_PATH%\stage\lib\* %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.3\release\address-model-64\architecture-x86\crypto-built-in\cxxstd-14-iso\dht-off\encryption-off\i2p-off\link-static\runtime-link-static\threading-multi\libtorrent-rasterbar.lib %LIB_PATH%

:end
cd %OLD_DIR%
