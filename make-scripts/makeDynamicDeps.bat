:Compiles Boost and libtorrent. Copies libs to lib folder.
set OLD_DIR=%cd%
cd %~dp0
cd ..\..\
set BIN_PATH=%cd%\afi-sync\bin
set SRC_PATH=%cd%\src
set LIB_PATH=%cd%\lib
set BOOST_BUILD_PATH=%SRC_PATH%\boost_1_66_0
set LIBTORRENT_BUILD_PATH=%SRC_PATH%\libtorrent-rasterbar-1.1.7
set PARAMS=--with-system --with-date_time --with-atomic --with-random --with-log --with-filesystem --with-thread architecture=x86 address-model=64 define=BOOST_USE_WINAPI_VERSION=0x0501
set MSVC_PARAMS_DYNAMIC=%PARAMS% link=shared runtime-link=shared toolset=msvc runtime-debugging=on variant=debug
set MSVC_PARAMS_STATIC=%PARAMS% link=static runtime-link=static toolset=msvc variant=release %PARAMS%
set PATH=%BOOST_BUILD_PATH%;%PATH%;%BIN_PATH%

call vcvarsall.bat x86_amd64
cd %SRC_PATH%
rmdir /s %BOOST_BUILD_PATH%
rmdir /s %LIBTORRENT_BUILD_PATH%
7za x %BOOST_BUILD_PATH%.7z
7za x %LIBTORRENT_BUILD_PATH%.7z
mkdir ..\lib

goto msvc-dynamic

:msvc-dynamic
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2  %MSVC_PARAMS_DYNAMIC%

cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS_DYNAMIC%

:msvc-dynamic-install
copy %BOOST_BUILD_PATH%\stage\lib\* %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.0\debug\address-model-64\architecture-x86\threading-multi\torrent.dll %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.0\debug\address-model-64\architecture-x86\threading-multi\torrent.lib %LIB_PATH%

goto end

:msvc-static
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2  %MSVC_PARAMS_STATIC%

:msvc-static-libtorrent
cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS_STATIC%

:msvc-static-install
copy %BOOST_BUILD_PATH%\stage\lib\* %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.0\release\address-model-64\architecture-x86\link-static\runtime-link-static\threading-multi\libtorrent.lib %LIB_PATH%

goto end

:end
cd %OLD_DIR%
