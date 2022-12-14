cmake_minimum_required(VERSION 2.8.5)

project(fcitx-qt5)

set(FcitxQt5_VERSION 1.0.0)
set(REQUIRED_QT_VERSION 5.1.0)

find_package(ECM 1.4.0 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(FindPkgConfig)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core DBus Widgets)
find_package(Qt5Gui ${REQUIRED_QT_VERSION} REQUIRED Private)
pkg_check_modules(FCITX_UTILS fcitx-utils)
pkg_check_modules(FCITX_CONFIG fcitx-config)

set(FcitxQt5_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/FcitxQt5)
add_subdirectory(dbusaddons)
add_subdirectory(widgetsaddons)
add_subdirectory(platforminputcontext)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
