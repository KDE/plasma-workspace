project(plasma-workspace)
set(PROJECT_VERSION "5.4.90")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(QT_MIN_VERSION "5.5.0")
set(KF5_MIN_VERSION "5.15.0")
set(INSTALL_SDDM_THEME TRUE)
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Widgets Quick QuickWidgets Concurrent Test Script Network)
find_package(ECM 1.8.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
                    Plasma DocTools Runner JsEmbed NotifyConfig Su NewStuff Wallet KCMUtils
                    IdleTime Declarative TextWidgets KDELibs4Support Crash GlobalAccel
                    NetworkManagerQt DBusAddons)
find_package(KF5XmlRpcClient REQUIRED)

# WARNING PlasmaQuick provides unversioned CMake config
find_package(KF5 REQUIRED COMPONENTS PlasmaQuick)
find_package(KF5 REQUIRED COMPONENTS SysGuard)
# Require KScreen >= 5.0.93 due to large API change
find_package(KF5 5.0.93 REQUIRED COMPONENTS Screen)
find_package(KF5 REQUIRED COMPONENTS Package)
# Baloo has a different version scheme than KF5 for now
find_package(KF5 5.1 REQUIRED COMPONENTS Baloo)
find_package(KF5TextEditor)
find_package(KWinDBusInterface CONFIG REQUIRED)

find_package(KScreenLocker REQUIRED)
find_package(ScreenSaverDBusInterface CONFIG REQUIRED)

find_package(Phonon4Qt5 4.6.60 REQUIRED NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMPackageConfigHelpers)
include(ECMMarkNonGuiExecutable)
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckIncludeFiles)
include(FeatureSummary)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)

find_package(KF5Activities ${KF5_MIN_VERSION})
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

find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                        URL "http://www.x.org"
                        TYPE OPTIONAL
                        PURPOSE "Required for building the X11 based workspace")

if(X11_FOUND)
  find_package(XCB MODULE REQUIRED COMPONENTS XCB KEYSYMS)
  set_package_properties(XCB PROPERTIES TYPE REQUIRED)
  if(NOT X11_SM_FOUND)
    message(FATAL_ERROR "\nThe X11 Session Management (SM) development package could not be found.\nPlease install libSM.\n")
  endif(NOT X11_SM_FOUND)

  add_feature_info("XInput" X11_Xinput_FOUND "Required for grabbing XInput2 devices in the screen locker")

  find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS X11Extras)
endif()

if(X11_FOUND AND XCB_XCB_FOUND)
  set(HAVE_X11 1)
endif()

find_package(KF5Wayland CONFIG)
set_package_properties(KF5Wayland PROPERTIES
                       TYPE REQUIRED
                       PURPOSE "Required for building screenlocker")
find_package(WaylandScanner)
find_package(Wayland 1.3 COMPONENTS Client Server)
set_package_properties(Wayland PROPERTIES
                       TYPE REQUIRED
                       PURPOSE "Required for building screenlocker")

include(ConfigureChecks.cmake)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

configure_file(config-workspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-workspace.h)
configure_file(config-unix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-unix.h )
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

configure_file(plasma.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/plasma.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/xsessions
)

plasma_install_package(lookandfeel org.kde.breeze.desktop look-and-feel lookandfeel)

if (INSTALL_SDDM_THEME)
# Install the login theme into the SDDM directory
# Longer term we need to look at making SDDM load from look and feel somehow.. and allow copying at runtime
    #NOTE this trailing slash is important to rename the directory
    install(DIRECTORY lookandfeel/contents/loginmanager/ DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze PATTERN "README.txt" EXCLUDE)
    install(DIRECTORY lookandfeel/contents/components DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze PATTERN "README.txt" EXCLUDE)
endif()

add_subdirectory(doc)
add_subdirectory(libkworkspace)
add_subdirectory(libtaskmanager)
add_subdirectory(components)

if(dbusmenu-qt5_FOUND)
  add_subdirectory(appmenu)
endif()

add_subdirectory(plasma-windowed)
add_subdirectory(shell)
add_subdirectory(freespacenotifier)
add_subdirectory(klipper)
add_subdirectory(krunner)
add_subdirectory(ksmserver)
add_subdirectory(ksplash)
add_subdirectory(systemmonitor)
add_subdirectory(statusnotifierwatcher)
add_subdirectory(startkde)
add_subdirectory(themes)

add_subdirectory(containmentactions)
add_subdirectory(runners)
add_subdirectory(applets)
add_subdirectory(dataengines)
add_subdirectory(wallpapers)

add_subdirectory(kioslave)
add_subdirectory(ktimezoned)
add_subdirectory(kuiserver)
add_subdirectory(menu)
add_subdirectory(phonon)
add_subdirectory(solidautoeject)
add_subdirectory(drkonqi)

ecm_optional_add_subdirectory(xembed-sni-proxy)

add_subdirectory(soliduiserver)
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
