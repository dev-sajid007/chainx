cmake_minimum_required(VERSION 3.21)

project(chainx-browser
    VERSION 1.0.0
    DESCRIPTION "ChainX Browser - a privacy-focused Chromium browser"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    WebEngineWidgets)

qt_standard_project_setup()

set(CHAINX_CORE_SOURCES
    src/app/Application.cpp
    src/app/Application.hpp

    src/browser/BrowserController.cpp
    src/browser/BrowserController.hpp
    src/browser/BrowserTab.cpp
    src/browser/BrowserTab.hpp
    src/browser/BrowserWindow.cpp
    src/browser/BrowserWindow.hpp
    src/browser/BookmarkManagerDialog.cpp
    src/browser/BookmarkManagerDialog.hpp
    src/browser/HistoryDialog.cpp
    src/browser/HistoryDialog.hpp
    src/browser/SettingsDialog.cpp
    src/browser/SettingsDialog.hpp

    src/database/Database.cpp
    src/database/Database.hpp

    src/navigation/AddressBar.cpp
    src/navigation/AddressBar.hpp

    src/services/BookmarkService.cpp
    src/services/BookmarkService.hpp
    src/services/DownloadService.cpp
    src/services/DownloadService.hpp
    src/services/HistoryService.cpp
    src/services/HistoryService.hpp

    src/settings/Settings.cpp
    src/settings/Settings.hpp

    src/utils/Logger.cpp
    src/utils/Logger.hpp
    src/utils/UrlUtils.cpp
    src/utils/UrlUtils.hpp

    src/web3/Web3Provider.cpp
    src/web3/Web3Provider.hpp
)

add_library(chainx_core STATIC ${CHAINX_CORE_SOURCES})

target_include_directories(chainx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chainx_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::WebEngineWidgets)

target_compile_definitions(chainx_core PUBLIC CHAINX_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(chainx_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(chainx-browser
    src/main.cpp
    assets/assets.qrc)

target_link_libraries(chainx-browser PRIVATE chainx_core)

set_target_properties(chainx-browser PROPERTIES
    OUTPUT_NAME chainx-browser
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE)

install(TARGETS chainx-browser
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin)

install(FILES packaging/chainx-browser.desktop
    DESTINATION share/applications)

install(FILES assets/icons/branding/chainx-256.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME chainx-browser.png)

install(FILES assets/icons/branding/chainx-512.png
    DESTINATION share/icons/hicolor/512x512/apps
    RENAME chainx-browser.png)

option(CHAINX_BUILD_TESTS "Build ChainX unit tests" ON)

if(CHAINX_BUILD_TESTS)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "GoogleTest not found - unit tests will not be built")
    endif()
endif()

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "chainx-browser")
    set(CPACK_PACKAGE_VENDOR "ChainX")
    set(CPACK_PACKAGE_CONTACT "ChainX Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Privacy-focused Chromium browser")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "web")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6webenginewidgets6 (>= 6.4), libqt6sql6-sqlite")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    include(CPack)
endif()
