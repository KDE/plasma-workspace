project(plasma-workspace)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(QT_MIN_VERSION "5.2.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Widgets Quick Test)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(KF5 CONFIG REQUIRED COMPONENTS
                    Plasma DocTools Runner JsEmbed NotifyConfig Su NewStuff
                    IdleTime ThreadWeaver Declarative PlasmaQuick KDE4Support)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}) #TODO: remove after splitting
    find_package(KF5 CONFIG REQUIRED COMPONENTS SysGuard)
    find_package(KWinDBusInterface CONFIG REQUIRED)
else()
    set(KWIN_INTERFACE  ${CMAKE_SOURCE_DIR}/kwin/org.kde.KWin.xml)
endif()

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMPackageConfigHelpers)
include(ECMMarkNonGuiExecutable)
include(ECMPackageConfigHelpers)
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckIncludeFiles)
include(FeatureSummary)

find_package(KF5Activities ${KF5_VERSION})
set_package_properties(KF5Activities PROPERTIES DESCRIPTION "management of Plasma activities"
                       TYPE OPTIONAL
                       PURPOSE "Needed by activity related plasmoids."
                      )

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "http://www.zlib.net"
                       TYPE REQUIRED
                      )

find_package(dbusmenu-qt5 CONFIG)
set_package_properties(dbusmenu-qt5 PROPERTIES DESCRIPTION "Support for notification area menus via the DBusMenu protocol"
                       URL "https://launchpad.net/libdbusmenu-qt"
                       TYPE OPTIONAL
                      )

find_package(QImageBlitz QUIET)
set_package_properties(QImageBlitz PROPERTIES DESCRIPTION "An image effects library"
                       URL "http://sourceforge.net/projects/qimageblitz"
                       TYPE OPTIONAL
                      )

find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                        URL "http://www.x.org"
                        TYPE OPTIONAL
                        PURPOSE "Required for building the X11 based workspace")

find_package(KdepimLibs 4.70.10 QUIET CONFIG)
set_package_properties(KdepimLibs PROPERTIES DESCRIPTION "The KDEPIM libraries"
                       URL "http://pim.kde.org"
                       TYPE OPTIONAL
                       PURPOSE "Required to build certain Plasma DataEngines (Akonadi, Calendar)"
                      )

find_package(Akonadi QUIET CONFIG)
set_package_properties(Akonadi PROPERTIES DESCRIPTION "An extensible cross-desktop storage service for PIM data"
                       URL "http://pim.kde.org/akonadi"
                       TYPE OPTIONAL
                       PURPOSE "Required to build certain Plasma DataEngines (Akonadi, Calendar)"
                      )

find_package(Boost 1.34.0 MODULE)
set_package_properties(Boost PROPERTIES DESCRIPTION "Boost C++ Libraries"
                       URL "http://www.boost.org"
                       TYPE OPTIONAL
                       PURPOSE "Required to build certain Plasma DataEngines (Akonadi, RSS, Calendar)"
                      )

if(X11_FOUND)
  find_package(XCB REQUIRED COMPONENTS XCB XFIXES DAMAGE COMPOSITE SHAPE SYNC RENDER RANDR KEYSYMS IMAGE SHM XTEST)
  set_package_properties(XCB PROPERTIES TYPE REQUIRED)
  find_package(XCB COMPONENTS ICCCM)
  add_feature_info("XCB-ICCCM" XCB_ICCCM_FOUND "Required for building test applications for KWin")

  find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS X11Extras)
endif()

include(ConfigureChecks.cmake)

set(PLASMAWORKSPACE_VERSION_MAJOR 4)
set(PLASMAWORKSPACE_VERSION_MINOR 95)
set(PLASMAWORKSPACE_VERSION_PATCH 0)
set(PLASMAWORKSPACE_VERSION_STRING "${PLASMAWORKSPACE_VERSION_MAJOR}.${PLASMAWORKSPACE_VERSION_MINOR}.${PLASMAWORKSPACE_VERSION_PATCH}" )

set(GENERIC_LIB_SOVERSION ${PLASMAWORKSPACE_VERSION_MAJOR})
set(GENERIC_LIB_VERSION "${PLASMAWORKSPACE_VERSION_STRING}")


include_directories("${CMAKE_CURRENT_BINARY_DIR}")

configure_file(config-workspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-workspace.h)
configure_file(config-unix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-unix.h )
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

plasma_install_package(lookandfeel org.kde.lookandfeel plasma/look-and-feel lookandfeel)

add_subdirectory(doc)
add_subdirectory(libkworkspace)
add_subdirectory(libtaskmanager)

add_subdirectory(appmenu)
add_subdirectory(freespacenotifier)
add_subdirectory(klipper)
add_subdirectory(krunner)
add_subdirectory(ksmserver)
add_subdirectory(ksplash)
add_subdirectory(statusnotifierwatcher)
add_subdirectory(startkde)
add_subdirectory(themes)

add_subdirectory(containmentactions)
add_subdirectory(runners)
add_subdirectory(applets)
add_subdirectory(dataengines)
add_subdirectory(wallpapers)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
