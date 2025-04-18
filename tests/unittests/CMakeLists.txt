cmake_minimum_required(VERSION 3.16)

project(app LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Network
    Widgets
)
find_package(OpenSSL REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/pbochecker/lib ${CMAKE_CURRENT_BINARY_DIR}/pbochecker)
add_executable(unittests WIN32
    ${PBOCHECKER_LIB_SOURCES}
    ../../app/src/model/pbocheckerprinterboost.cpp ../../app/src/model/pbocheckerprinterboost.h
    ../../app/src/model/afisync.cpp ../../app/src/model/afisync.h
    ../../app/src/model/afisynclogger.cpp ../../app/src/model/afisynclogger.h
    ../../app/src/model/apis/isync.h
    ../../app/src/model/apis/libtorrent/ahasher.cpp ../../app/src/model/apis/libtorrent/ahasher.h
    ../../app/src/model/apis/libtorrent/alerthandler.cpp ../../app/src/model/apis/libtorrent/alerthandler.h
    ../../app/src/model/apis/libtorrent/deltadownloader.cpp ../../app/src/model/apis/libtorrent/deltadownloader.h
    ../../app/src/model/apis/libtorrent/deltamanager.cpp ../../app/src/model/apis/libtorrent/deltamanager.h
    ../../app/src/model/apis/libtorrent/deltapatcher.cpp ../../app/src/model/apis/libtorrent/deltapatcher.h
    ../../app/src/model/apis/libtorrent/directorywatcher.cpp ../../app/src/model/apis/libtorrent/directorywatcher.h
    ../../app/src/model/apis/libtorrent/libtorrentapi.cpp ../../app/src/model/apis/libtorrent/libtorrentapi.h
    ../../app/src/model/apis/libtorrent/speedcalculator.cpp ../../app/src/model/apis/libtorrent/speedcalculator.h
    ../../app/src/model/apis/libtorrent/storagemovemanager.cpp ../../app/src/model/apis/libtorrent/storagemovemanager.h
    ../../app/src/model/cihash.h
    ../../app/src/model/console.cpp ../../app/src/model/console.h
    ../../app/src/model/customdebug.h
    ../../app/src/model/deletable.cpp ../../app/src/model/deletable.h
    ../../app/src/model/deletabledetector.cpp ../../app/src/model/deletabledetector.h
    ../../app/src/model/destructionwaiter.cpp ../../app/src/model/destructionwaiter.h
    ../../app/src/model/fileutils.cpp ../../app/src/model/fileutils.h
    ../../app/src/model/global.cpp ../../app/src/model/global.h
    ../../app/src/model/installer.cpp ../../app/src/model/installer.h
    ../../app/src/model/interfaces/ibandwidthmeter.h
    ../../app/src/model/interfaces/imod.h
    ../../app/src/model/interfaces/irepository.h
    ../../app/src/model/interfaces/isettings.h
    ../../app/src/model/interfaces/isyncitem.h
    ../../app/src/model/jsonreader.cpp ../../app/src/model/jsonreader.h
    ../../app/src/model/jsonutils.cpp ../../app/src/model/jsonutils.h
    ../../app/src/model/mod.cpp ../../app/src/model/mod.h
    ../../app/src/model/modadapter.cpp ../../app/src/model/modadapter.h
    ../../app/src/model/paths.cpp ../../app/src/model/paths.h
    ../../app/src/model/processmonitor.cpp ../../app/src/model/processmonitor.h
    ../../app/src/model/repository.cpp ../../app/src/model/repository.h
    ../../app/src/model/settingsmodel.cpp ../../app/src/model/settingsmodel.h
    ../../app/src/model/syncitem.cpp ../../app/src/model/syncitem.h
    ../../app/src/model/syncnetworkaccessmanager.cpp ../../app/src/model/syncnetworkaccessmanager.h
    ../../app/src/model/szip.cpp ../../app/src/model/szip.h
    ../../app/src/model/treemodel.cpp ../../app/src/model/treemodel.h
    ../../app/src/model/version.h
    ../../app/src/model/versioncheckresult.cpp ../../app/src/model/versioncheckresult.h
    ../../deps/googletest/googlemock/src/gmock-all.cc
    ../../deps/googletest/googletest/src/gtest-all.cc
    afisynctest.cpp
    deletabledetectortest.cpp
    main.cpp
    testconstants.h
)
target_include_directories(unittests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/pbochecker/lib
    ../../app/src
    ../../deps/googletest/googlemock
    ../../deps/googletest/googlemock/include
    ../../deps/googletest/googletest
    ../../deps/googletest/googletest/include
)

target_compile_definitions(unittests PRIVATE
    GTEST_LANG_CXX11
)

target_link_libraries(unittests PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Network
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(WIN32)
    target_include_directories(unittests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/boost_1_87_0
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/libtorrent-rasterbar-2.0.11/include
    )

    target_compile_definitions(unittests PRIVATE
        _WIN32_WINNT=0x0A00
    )

    target_compile_definitions(unittests PRIVATE
        BOOST_ALL_DYN_LINK
        TORRENT_LINKING_SHARED
    )

    set(LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib")
    set(POSTFIX "vc143-mt-gd-x64-1_87")
    target_link_libraries(unittests PRIVATE
        ${LIB_PATH}/boost_atomic-${POSTFIX}.lib
        ${LIB_PATH}/boost_date_time-${POSTFIX}.lib
        ${LIB_PATH}/boost_filesystem-${POSTFIX}.lib
        ${LIB_PATH}/boost_log-${POSTFIX}.lib
        ${LIB_PATH}/boost_log_setup-${POSTFIX}.lib
        ${LIB_PATH}/boost_random-${POSTFIX}.lib
        ${LIB_PATH}/boost_system-${POSTFIX}.lib
        ${LIB_PATH}/boost_thread-${POSTFIX}.lib
        ${LIB_PATH}/torrent.lib
        ws2_32
    )
endif()

if(UNIX)
    target_compile_definitions(unittests PRIVATE
        BOOST_LOG_DYN_LINK
    )

    target_link_libraries(unittests PRIVATE
        boost_atomic
        boost_date_time
        boost_filesystem
        boost_log
        boost_log_setup
        boost_random
        boost_system
        boost_thread
        torrent-rasterbar
    )
endif()

install(TARGETS unittests
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
