cmake_minimum_required(VERSION 3.16)

project(app LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(OpenSSL REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Network
    Widgets
)

if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../deps/pbochecker/lib ${CMAKE_CURRENT_BINARY_DIR}/pbochecker)

add_executable(AFISync WIN32
    ${PBOCHECKER_LIB_SOURCES}
    src/model/pbocheckerprinterboost.cpp src/model/pbocheckerprinterboost.h
    ../deps/try_signal/signal_error_code.cpp ../deps/try_signal/signal_error_code.hpp
    ../deps/try_signal/try_signal.cpp ../deps/try_signal/try_signal.hpp
    src/model/afisync.cpp src/model/afisync.h
    src/model/afisynclogger.cpp src/model/afisynclogger.h
    src/model/apis/isync.h
    src/model/apis/libtorrent/ahasher.cpp src/model/apis/libtorrent/ahasher.h
    src/model/apis/libtorrent/alerthandler.cpp src/model/apis/libtorrent/alerthandler.h
    src/model/apis/libtorrent/deltadownloader.cpp src/model/apis/libtorrent/deltadownloader.h
    src/model/apis/libtorrent/deltamanager.cpp src/model/apis/libtorrent/deltamanager.h
    src/model/apis/libtorrent/deltapatcher.cpp src/model/apis/libtorrent/deltapatcher.h
    src/model/apis/libtorrent/directorywatcher.cpp src/model/apis/libtorrent/directorywatcher.h
    src/model/apis/libtorrent/libtorrentapi.cpp src/model/apis/libtorrent/libtorrentapi.h
    src/model/apis/libtorrent/speedcalculator.cpp src/model/apis/libtorrent/speedcalculator.h
    src/model/apis/libtorrent/storagemovemanager.cpp src/model/apis/libtorrent/storagemovemanager.h
    src/model/cihash.h
    src/model/console.cpp src/model/console.h
    src/model/deletable.cpp src/model/deletable.h
    src/model/deletabledetector.cpp src/model/deletabledetector.h
    src/model/destructionwaiter.cpp src/model/destructionwaiter.h
    src/model/fileutils.cpp src/model/fileutils.h
    src/model/global.cpp src/model/global.h
    src/model/installer.cpp src/model/installer.h
    src/model/interfaces/ibandwidthmeter.h
    src/model/interfaces/imod.h
    src/model/interfaces/irepository.h
    src/model/interfaces/isettings.h
    src/model/interfaces/isyncitem.h
    src/model/jsonreader.cpp src/model/jsonreader.h
    src/model/jsonutils.cpp src/model/jsonutils.h
    src/model/main.cpp
    src/model/mod.cpp src/model/mod.h
    src/model/modadapter.cpp src/model/modadapter.h
    src/model/paths.cpp src/model/paths.h
    src/model/processmonitor.cpp src/model/processmonitor.h
    src/model/repository.cpp src/model/repository.h
    src/model/settingsmodel.cpp src/model/settingsmodel.h
    src/model/syncitem.cpp src/model/syncitem.h
    src/model/syncnetworkaccessmanager.cpp src/model/syncnetworkaccessmanager.h
    src/model/szip.cpp src/model/szip.h
    src/model/treemodel.cpp src/model/treemodel.h
    src/model/version.h
    src/model/versioncheckresult.cpp src/model/versioncheckresult.h
    src/view/asbannerbar.cpp src/view/asbannerbar.h src/view/asbannerbar.ui
    src/view/assettingsview.cpp src/view/assettingsview.h src/view/assettingsview.ui
    src/view/astreeitem.cpp src/view/astreeitem.h
    src/view/astreewidget.cpp src/view/astreewidget.h
    src/view/mainwindow.cpp src/view/mainwindow.h src/view/mainwindow.ui
    src/view/optionalsetting.cpp src/view/optionalsetting.h src/view/optionalsetting.ui
    src/view/pathsetting.cpp src/view/pathsetting.h src/view/pathsetting.ui
    src/view/app.rc
)

target_include_directories(AFISync PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/pbochecker/lib
    ${LIBTORRENT_PATH}/include
    src
)

target_link_libraries(AFISync PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
    OpenSSL::SSL
    OpenSSL::Crypto
)

set(resources_resource_files
    "src/view/afisync_header.png"
)

qt_add_resources(AFISync "resources"
    PREFIX
        "/"
    BASE
        "src/view"
    FILES
        ${resources_resource_files}
)

if(WIN32)
    target_include_directories(AFISync PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/boost_1_87_0
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libtorrent-rasterbar-2.0.11/include
    )

    target_compile_definitions(AFISync PRIVATE
        _WIN32_WINNT=0x0A00
    )

    set(LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
        set(POSTFIX "vc143-mt-s-x64-1_87")
        target_link_libraries(AFISync PRIVATE
            bcrypt
            ${LIB_PATH}/libboost_atomic-${POSTFIX}.lib
            ${LIB_PATH}/libboost_random-${POSTFIX}.lib
            ${LIB_PATH}/libboost_system-${POSTFIX}.lib
            ${LIB_PATH}/libboost_log-${POSTFIX}.lib
            ${LIB_PATH}/libboost_log_setup-${POSTFIX}.lib
            ${LIB_PATH}/libboost_filesystem-${POSTFIX}.lib
            ${LIB_PATH}/libboost_thread-${POSTFIX}.lib
            ${LIB_PATH}/libboost_chrono-${POSTFIX}.lib
            ${LIB_PATH}/libtorrent-rasterbar.lib
            ws2_32
        )
        add_custom_command(TARGET AFISync POST_BUILD
            COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/AFISync.exe ${CMAKE_CURRENT_BINARY_DIR}/afisync_cmd.exe
            COMMAND mt.exe -manifest ${CMAKE_CURRENT_SOURCE_DIR}/manifest.xml -outputresource:${CMAKE_CURRENT_BINARY_DIR}/AFISync.exe
        )
    else()
        target_compile_definitions(AFISync PRIVATE
           BOOST_ALL_DYN_LINK
           TORRENT_LINKING_SHARED
        )
        set(POSTFIX "vc143-mt-gd-x64-1_87")
        target_link_libraries(AFISync PRIVATE
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
endif()

if(UNIX)
    target_compile_definitions(AFISync PRIVATE
        BOOST_LOG_DYN_LINK
    )

    target_link_libraries(AFISync PRIVATE
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
    if (DEFINED LIBTORRENT_PATH)
        target_link_libraries(AFISync PRIVATE
            ${LIBTORRENT_PATH}/build/libtorrent-rasterbar.so
        )
    else()
        target_link_libraries(AFISync PRIVATE
            torrent-rasterbar
        )
        message("system libTorrent in use")
    endif()
endif()

install(TARGETS AFISync
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
