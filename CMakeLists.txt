if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

project(ubuntu-sdk-ide C CXX)

include(ExternalProject)
include(ProcessorCount)

function(ConcatStrings targetVar )
  set(loc_val ${${targetVar}})
  foreach(curr_substr ${ARGN})
      set(loc_val "${loc_val}${curr_substr}")
  endforeach()
  set(${targetVar} ${loc_val} PARENT_SCOPE)
endfunction()

ProcessorCount(JOBS)
if(NOT JOBS EQUAL 0)
  set(GENERATED_MAKE_FLAGS -j${JOBS})
endif()

cmake_minimum_required(VERSION 2.8.9)
set (ubuntu_sdk_VERSION_MAJOR 3)
set (ubuntu_sdk_VERSION_MINOR 5)

set (QT_VERSION      "qt-5.6.1")
set (EXT_SOURCE_DIR  "${CMAKE_SOURCE_DIR}/dist")
set (QT_SOURCE_DIR   "${EXT_SOURCE_DIR}/qt5")
set (QT_BUILD_DIR    "${CMAKE_BINARY_DIR}/qt-build/build")
set (QT_INSTALL_DIR  "${CMAKE_BINARY_DIR}/qt")

add_custom_target(RevertPatches
                  env QUILT_PATCHES=debian/patches quilt pop -a
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(QtSource
                  ${CMAKE_SOURCE_DIR}/get_git_source.py -k -d "${QT_SOURCE_DIR}" -b "5.6.1" "http://code.qt.io/qt/qt5.git"
                  COMMAND cd ${QT_SOURCE_DIR} && ./init-repository && cd ${CMAKE_SOURCE_DIR}
                  COMMAND ${CMAKE_SOURCE_DIR}/get_package_source.py -c ${QT_SOURCE_DIR} qtfeedback-opensource-src:qtfeedback qtpim-opensource-src:qtpim
                  COMMAND ${CMAKE_SOURCE_DIR}/get_bzr_source.py -d ${QT_SOURCE_DIR}/ubuntu-sdk lp:ubuntu-ui-toolkit/staging
                  COMMAND cat ${CMAKE_SOURCE_DIR}/patches/qt-enable-uitk.patch >> ${QT_SOURCE_DIR}/.gitmodules
                  COMMAND ${CMAKE_SOURCE_DIR}/fix-qt-build.py -b \"${QT_SOURCE_DIR}\"
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(LibDusSource
                  ${CMAKE_SOURCE_DIR}/get_bzr_source.py -d "${EXT_SOURCE_DIR}/libdbusmenu-qt" lp:libdbusmenu-qt
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(MaliitSource
                  ${CMAKE_SOURCE_DIR}/get_package_source.py maliit-framework
                  WORKING_DIRECTORY ${EXT_SOURCE_DIR})

add_custom_target(fcitx-qtSource
                  ${CMAKE_SOURCE_DIR}/get_bzr_source.py -d "${EXT_SOURCE_DIR}/fcitx-qt" lp:ubuntu/wily-proposed/fcitx-qt5
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(AppmenuSource
                  ${CMAKE_SOURCE_DIR}/get_bzr_source.py -d "${EXT_SOURCE_DIR}/appmenu-qt5"  lp:appmenu-qt5
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(QtCreatorSource
                  ${CMAKE_SOURCE_DIR}/get_git_source.py -d "${EXT_SOURCE_DIR}/qtcreator" -b "v4.1.0" "http://code.qt.io/qt-creator/qt-creator.git"
                  COMMAND ${CMAKE_SOURCE_DIR}/get_bzr_source.py -d "${EXT_SOURCE_DIR}/qtcreator/src/plugins/ubuntu" lp:qtcreator-plugin-ubuntu
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(YamlCppSource
                  ${CMAKE_SOURCE_DIR}/get_package_source.py yaml-cpp
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(sources DEPENDS QtSource LibDusSource MaliitSource fcitx-qtSource AppmenuSource QtCreatorSource YamlCppSource)

add_custom_command(TARGET sources
                   PRE_BUILD
                   COMMAND "REVERTING ALL PATCHES"
                   COMMAND quilt pop -a
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message("debug mode enabled")
    set(QT_ADDITIONAL_CONF "-debug")
    set(QMAKE_ADDITIONAL_CONF "CONFIG+=DEBUG")
    set(CMAKE_ADDITIONAL_CONF "-DCMAKE_BUILD_TYPE=Debug")
    set(CMAKE_BUILD_TYPE_STRING "Debug")
ELSE(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_BUILD_TYPE_STRING "RelWithDebInfo")
    set(QT_ADDITIONAL_CONF "-force-debug-info")
    set(QMAKE_ADDITIONAL_CONF "CONFIG+=force_debug_info")
    set(CMAKE_ADDITIONAL_CONF "-DCMAKE_BUILD_TYPE=RelWithDebInfo")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

add_custom_target(ApplyPatches
                  ${CMAKE_SOURCE_DIR}/apply-patches.sh
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

ExternalProject_Add(${QT_VERSION}
  PREFIX      "qt-build"
  SOURCE_DIR  "${QT_SOURCE_DIR}"
  BINARY_DIR  "${QT_BUILD_DIR}"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "${CMAKE_SOURCE_DIR}/fix-qt-build.py -f \"${QT_SOURCE_DIR}\" && ${QT_SOURCE_DIR}/configure -prefix ${QT_INSTALL_DIR} ${QT_ADDITIONAL_CONF} `cat ${CMAKE_SOURCE_DIR}/qt5config`"
  BUILD_COMMAND     sh -c "make ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "make install"
  DEPENDS     ApplyPatches
)

SET (DBUSMENU_PREFIX "${CMAKE_BINARY_DIR}/libdbusmenu-qt-build")
ConcatStrings (DBUSMENU_BUILD_SCRIPT
  "${CMAKE_COMMAND} "
  "-DCMAKE_INSTALL_PREFIX=${QT_INSTALL_DIR} "
  "-DCMAKE_PREFIX_PATH=${QT_INSTALL_DIR} "
  "-DUSE_QT5=true "
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_STRING} "
  "-DCMAKE_INSTALL_LIBDIR=lib ${EXT_SOURCE_DIR}/libdbusmenu-qt "
)

ExternalProject_Add(libdbusmenu-qt
  DEPENDS ${QT_VERSION}
  PREFIX      "${DBUSMENU_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/libdbusmenu-qt"
  BINARY_DIR  "${DBUSMENU_PREFIX}/build"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "${DBUSMENU_BUILD_SCRIPT}"
  BUILD_COMMAND     sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make  ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make install"
)

ConcatStrings (LIBYAML_BUILD_SCRIPT
  "${CMAKE_COMMAND} "
  "-DCMAKE_INSTALL_PREFIX=${QT_INSTALL_DIR} "
  "-DCMAKE_PREFIX_PATH=${QT_INSTALL_DIR} "
  "-DCMAKE_BUILD_TYPE=DEBUG" #${CMAKE_BUILD_TYPE_STRING} "
  "-DCMAKE_INSTALL_LIBDIR=lib ${EXT_SOURCE_DIR}/yaml-cpp "
  "-DCMAKE_C_FLAGS=-fPIC "
  "-DCMAKE_CXX_FLAGS=-fPIC "
)

SET (LIBYAML_PREFIX "${CMAKE_BINARY_DIR}/yaml-cpp")
ExternalProject_Add(libyaml-cpp
  DEPENDS ${QT_VERSION}
  PREFIX      "${LIBYAML_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/yaml-cpp"
  BINARY_DIR  "${LIBYAML_PREFIX}/build"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "${LIBYAML_BUILD_SCRIPT}"
  BUILD_COMMAND     sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make  ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make install"
)

SET (MALIITPLUGIN_PREFIX "${CMAKE_BINARY_DIR}/maliit-plugin-qt-build")
ExternalProject_Add(maliit-plugin
  DEPENDS     ${QT_VERSION}
  PREFIX      "${MALIITPLUGIN_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/maliit-framework"
  BINARY_DIR  "${MALIITPLUGIN_PREFIX}/maliit-framework"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib ${QT_INSTALL_DIR}/bin/qmake -r CONFIG+=\"qt5-inputcontext glib nodoc notests warn_off\" ${QMAKE_ADDITIONAL_CONF}  M_IM_PREFIX=${QT_INSTALL_DIR} MALIIT_SERVER_ARGUMENTS=\"-software -bypass-wm-hint\" MALIIT_DEFAULT_PLUGIN=libmaliit-keyboard-plugin.so ${EXT_SOURCE_DIR}/maliit-framework"
  BUILD_COMMAND     sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make  ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "cd ${MALIITPLUGIN_PREFIX}/maliit-framework/input-context && make install"
)

SET (FCITX_PREFIX "${CMAKE_BINARY_DIR}/fcitx-qt-build")
ConcatStrings (FCITX_BUILD_SCRIPT
  "${CMAKE_COMMAND} "
  "-DCMAKE_INSTALL_PREFIX=${QT_INSTALL_DIR} "
  "-DCMAKE_PREFIX_PATH=${QT_INSTALL_DIR} "
  "-DUSE_QT5=true "
  "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE_STRING} "
  "-DCMAKE_INSTALL_LIBDIR=lib ${EXT_SOURCE_DIR}/fcitx-qt"
)
ExternalProject_Add(fcitx-qt5
  DEPENDS ${QT_VERSION}
  PREFIX      "${FCITX_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/fcitx-qt"
  BINARY_DIR  "${FCITX_PREFIX}/build"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "${FCITX_BUILD_SCRIPT}"
  BUILD_COMMAND     sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib VERBOSE=1 make ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib VERBOSE=1 make install"
)

SET (APPMENU_PREFIX "${CMAKE_BINARY_DIR}/appmenu-qt5-build")
ExternalProject_Add(appmenu-qt5
  DEPENDS libdbusmenu-qt
  PREFIX      "${APPMENU_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/appmenu-qt5"
  BINARY_DIR  "${APPMENU_PREFIX}/build"
  INSTALL_DIR "${QT_INSTALL_DIR}"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "PKG_CONFIG_PATH=${QT_INSTALL_DIR}/lib/pkgconfig ${QT_INSTALL_DIR}/bin/qmake -r ${QMAKE_ADDITIONAL_CONF} ${EXT_SOURCE_DIR}/appmenu-qt5"
  BUILD_COMMAND     sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make  ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make install"
)

SET (QTCREATOR_PREFIX "${CMAKE_BINARY_DIR}/qtcreator-build")
ExternalProject_Add(qtcreator_3.5.0
  DEPENDS appmenu-qt5 fcitx-qt5 maliit-plugin libyaml-cpp
  PREFIX      "${QTCREATOR_PREFIX}"
  SOURCE_DIR  "${EXT_SOURCE_DIR}/qtcreator"
  BINARY_DIR  "${QTCREATOR_PREFIX}/build"
  INSTALL_DIR "${CMAKE_BINARY_DIR}/ubuntu-sdk-ide"
  DOWNLOAD_COMMAND  sh -c "exit 0"
  CONFIGURE_COMMAND sh -c "${QT_INSTALL_DIR}/bin/qmake -r USE_SYSTEM_BOTAN=1 CONFIG+=debug ${QMAKE_ADDITIONAL_CONF} ${EXT_SOURCE_DIR}/qtcreator"
  BUILD_COMMAND     sh -c "QT_PRIVATE_HEADERS=${QT_INSTALL_DIR} LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make ${GENERATED_MAKE_FLAGS}"
  INSTALL_COMMAND   sh -c "QT_PRIVATE_HEADERS=${QT_INSTALL_DIR} LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make INSTALL_ROOT=${CMAKE_BINARY_DIR}/ubuntu-sdk-ide -j1 docs deployqt"
)

#set(QTC_BUILD  "${CMAKE_BINARY_DIR}/ubuntu-sdk-ide")
#set(QTC_SOURCE "${QTCREATOR_PREFIX}/qtcreator")

#SET (QTCREATOR_GO_PREFIX "${CMAKE_BINARY_DIR}/qtcreator-plugin-go-build")
#ExternalProject_Add(qtcreator_plugin_go
#  DEPENDS qtcreator_3.5.0
#  PREFIX      "${QTCREATOR_GO_PREFIX}"
#  SOURCE_DIR  "${QTCREATOR_GO_PREFIX}/qtcreator-plugin-go"
#  BINARY_DIR  "${QTCREATOR_GO_PREFIX}/build"
#  INSTALL_DIR "${QTCREATOR_GO_PREFIX}/ubuntu-sdk-ide"
#  DOWNLOAD_COMMAND  sh -c "bzr branch lp:qtcreator-plugin-go ${QTCREATOR_GO_PREFIX}/qtcreator-plugin-go -Ossl.cert_reqs=none --use-existing-dir"
#  CONFIGURE_COMMAND sh -c "env QTC_BUILD=\"${QTC_BUILD}\" QTC_SOURCE=\"${QTC_SOURCE}\" ${QT_INSTALL_DIR}/bin/qmake -r ${QTCREATOR_GO_PREFIX}/qtcreator-plugin-go"
#  BUILD_COMMAND     sh -c "env QTC_BUILD=\"${QTC_BUILD}\" QTC_SOURCE=\"${QTC_SOURCE}\" QT_PRIVATE_HEADERS=${QT_INSTALL_DIR} LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make ${GENERATED_MAKE_FLAGS}"
#  #there is no install step required, the plugin is built directly into the installed plugins dir
#  INSTALL_COMMAND   sh -c "exit 0"
#)

# Disable the AP plugin for now
#
#SET (QTCREATOR_AP_PREFIX "${CMAKE_BINARY_DIR}/qtcreator-plugin-autopilot-build")
#ExternalProject_Add(qtcreator_plugin_autopilot
#  DEPENDS qtcreator_3.5.0
#  PREFIX      "${QTCREATOR_AP_PREFIX}"
#  SOURCE_DIR  "${QTCREATOR_AP_PREFIX}/qtcreator-plugin-autopilot"
#  BINARY_DIR  "${QTCREATOR_AP_PREFIX}/build"
#  INSTALL_DIR "${QTCREATOR_AP_PREFIX}/ubuntu-sdk-ide"
#  DOWNLOAD_COMMAND  sh -c "bzr branch lp:qtcreator-plugin-autopilot ${QTCREATOR_AP_PREFIX}/qtcreator-plugin-autopilot  -Ossl.cert_reqs=none --use-existing-dir"
#  CONFIGURE_COMMAND sh -c "env QTC_BUILD=\"${QTC_BUILD}\" QTC_SOURCE=\"${QTC_SOURCE}\" ${QT_INSTALL_DIR}/bin/qmake -r ${QTCREATOR_AP_PREFIX}/qtcreator-plugin-autopilot"
#  BUILD_COMMAND     sh -c "env QTC_BUILD=\"${QTC_BUILD}\" QTC_SOURCE=\"${QTC_SOURCE}\" QT_PRIVATE_HEADERS=${QT_INSTALL_DIR}  LD_LIBRARY_PATH=${QT_INSTALL_DIR}/lib make ${GENERATED_MAKE_FLAGS}"
#  #there is no install step required, the plugin is built directly into the installed plugins dir
#  INSTALL_COMMAND   sh -c "exit 0"
#)
