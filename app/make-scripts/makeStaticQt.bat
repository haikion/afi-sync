set OLD_DIR=%cd%
cd %~dp0%..\..
set VS_DIR=E:/Program Files/Microsoft Visual Studio/2022/Community

set ROOT_DIR=E:/qts
set SRC_DIR=%ROOT_DIR%/src
set BUILD_DIR=%ROOT_DIR%/build
set LIB_DIR=%ROOT_DIR%/lib
set INCLUDE_DIR=%ROOT_DIR%/include
set PATH=%VS_DIR%/VC/Auxiliary/Build;E:/Qt/6.8.0/msvc2022_64/bin;%ROOT_DIR%;%systemroot%;%systemroot%/System32;E:/Python/Python311;E:/Program Files/CMake/bin

call vcvarsall.bat x86_amd64
cd %SRC_DIR%\qt-everywhere-src-6.8.0

:compile
:nmake clean
call configure.bat -prefix %ROOT_DIR% -I %INCLUDE_DIR% -L %LIB_DIR% -platform win32-msvc -release -opensource -confirm-license -strip -no-shared -static -static-runtime -ltcg -make libs -make tools -nomake examples -nomake examples -no-dbus -no-qml-debug -no-icu -no-openssl -no-gtk -no-opengl -no-opengles3 -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtwebview -skip qtx11extras -skip qtxmlpatterns
cmake --build . --parallel

:install
ninja install
:jom -j 4
:jom -j 4 install
:nmake
:nmake install

:end
cd %OLD_DIR%
