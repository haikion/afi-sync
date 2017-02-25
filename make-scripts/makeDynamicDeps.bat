:Compiles Boost and libtorrent. Copies libs to lib folder.
set OLD_DIR=%cd%
cd %~dp0
set BIN_PATH=%cd%\bin
cd ..\..\
set SRC_PATH=%cd%\src
set LIB_PATH=%cd%\lib
set BOOST_BUILD_PATH=%SRC_PATH%\boost_1_63_0
set LIBTORRENT_BUILD_PATH=%SRC_PATH%\libtorrent-rasterbar-1.1.1
set PARAMS=--with-system --with-date_time --with-atomic --with-random architecture=x86 address-model=64
set PATH=C:\Qt\Tools\mingw530_32\bin;%BOOST_BUILD_PATH%;%PATH%;%BIN_PATH%

cd %SRC_PATH%
rmdir /s %BOOST_BUILD_PATH%
rmdir /s %LIBTORRENT_BUILD_PATH%
7za x %BOOST_BUILD_PATH%.7z
7za x %LIBTORRENT_BUILD_PATH%.7z
mkdir ..\lib

goto msvc-dynamic

:msvc-dynamic
set MSVC_PARAMS=%PARAMS% address-model=64 link=shared runtime-link=shared toolset=msvc runtime-debugging=on variant=debug 
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2  %MSVC_PARAMS%

cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS%

:msvc-dynamic-install
copy %BOOST_BUILD_PATH%\stage\lib\* %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.0\debug\address-model-64\architecture-x86\threading-multi\torrent.dll %LIB_PATH%
copy %LIBTORRENT_BUILD_PATH%\bin\msvc-14.0\debug\address-model-64\architecture-x86\threading-multi\torrent.lib %LIB_PATH%

goto end

:msvc-static
set MSVC_PARAMS=%PARAMS% link=static runtime-link=static toolset=msvc %PARAMS%
cd %BOOST_BUILD_PATH%
call bootstrap.bat
b2  %MSVC_PARAMS%

:msvc-static-libtorrent
cd %LIBTORRENT_BUILD_PATH%
b2 %MSVC_PARAMS%

goto end

:end
cd %OLD_DIR%