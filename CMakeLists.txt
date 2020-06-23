cmake_minimum_required(VERSION 3.0)

project(plasma-workspace)
set(PROJECT_VERSION "5.19.2")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.14.0")
set(KF5_MIN_VERSION "5.70.0")
set(INSTALL_SDDM_THEME TRUE)
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Widgets Quick QuickWidgets Concurrent Test Network)
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMMarkNonGuiExecutable)
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckIncludeFiles)
include(FeatureSummary)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(ECMQueryQmake)
include(KDEPackageAppTemplates)
include(KDEClangFormat)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
                    Plasma DocTools Runner Notifications NotifyConfig Su NewStuff Wallet
                    IdleTime Declarative I18n KCMUtils TextWidgets KDELibs4Support Crash GlobalAccel
                    DBusAddons Wayland CoreAddons People ActivitiesStats KIO)
find_package(KDED CONFIG REQUIRED)

find_package(KF5NetworkManagerQt ${KF5_MIN_VERSION})
set_package_properties(KF5NetworkManagerQt PROPERTIES DESCRIPTION "Qt wrapper for NetworkManager API"
                       TYPE OPTIONAL
                       PURPOSE "Needed by geolocation data engine."
                      )

find_package(KF5Kirigami2 ${KF5_MIN_VERSION} CONFIG)
set_package_properties(KF5Kirigami2 PROPERTIES
    DESCRIPTION "A QtQuick based components set"
    TYPE RUNTIME
)

find_package(KF5QuickCharts ${KF5_MIN_VERSION} CONFIG)
set_package_properties(KF5QuickCharts PROPERTIES
    DESCRIPTION "Used for rendering charts"
    TYPE RUNTIME
)

# WARNING PlasmaQuick provides unversioned CMake config
find_package(KUserFeedback)
find_package(KF5 REQUIRED COMPONENTS PlasmaQuick)
find_package(KSysGuard CONFIG REQUIRED)
find_package(KF5 REQUIRED COMPONENTS Package)

find_package(KF5Baloo)
set_package_properties(KF5Baloo PROPERTIES DESCRIPTION "File Searching"
                       TYPE RECOMMENDED
                       PURPOSE "Needed for the File Search runner."
                      )

find_package(KF5TextEditor)
find_package(KWinDBusInterface CONFIG REQUIRED)

find_package(KF5Screen CONFIG REQUIRED)
find_package(KScreenLocker 5.13.80 REQUIRED)
find_package(ScreenSaverDBusInterface CONFIG REQUIRED)
find_package(KF5Holidays)
set_package_properties(KF5Holidays PROPERTIES DESCRIPTION "Holidays provider for Plasma calendar"
                       TYPE OPTIONAL
                       PURPOSE "Needed to for holidays plugin for Plasma Calendar."
                      )

find_package(Phonon4Qt5 4.6.60 REQUIRED NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE REQUIRED)
find_package(KF5Activities ${KF5_MIN_VERSION})
set_package_properties(KF5Activities PROPERTIES DESCRIPTION "management of Plasma activities"
                       TYPE OPTIONAL
                       PURPOSE "Needed by activity related plasmoids."
                      )

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "https://www.zlib.net"
                       TYPE REQUIRED
                      )

find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                        URL "https://www.x.org"
                        TYPE OPTIONAL
                        PURPOSE "Required for building the X11 based workspace")

if(X11_FOUND)
  find_package(XCB MODULE REQUIRED COMPONENTS XCB RANDR)
  set_package_properties(XCB PROPERTIES TYPE REQUIRED)
  if(NOT X11_SM_FOUND)
    message(FATAL_ERROR "\nThe X11 Session Management (SM) development package could not be found.\nPlease install libSM.\n")
  endif(NOT X11_SM_FOUND)

  find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS X11Extras)
endif()

if(X11_FOUND AND XCB_XCB_FOUND)
  set(HAVE_X11 1)
endif()

find_package(AppStreamQt 0.10.6)
set_package_properties(AppStreamQt PROPERTIES DESCRIPTION "Access metadata for listing available software"
                       URL "https://www.freedesktop.org/wiki/Distributions/AppStream/"
		       TYPE OPTIONAL
                      )

if(${AppStreamQt_FOUND})
    set(HAVE_APPSTREAMQT true)
endif()

include(ConfigureChecks.cmake)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

configure_file(config-workspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-workspace.h)
configure_file(config-unix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-unix.h )
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)
configure_file(config-appstream.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-appstream.h )
add_subdirectory(login-sessions)

plasma_install_package(lookandfeel org.kde.breeze.desktop look-and-feel lookandfeel)

if (INSTALL_SDDM_THEME)
    configure_file(sddm-theme/theme.conf.cmake ${CMAKE_CURRENT_BINARY_DIR}/sddm-theme/theme.conf)

# Install the login theme into the SDDM directory
# Longer term we need to look at making SDDM load from look and feel somehow.. and allow copying at runtime
    #NOTE this trailing slash is important to rename the directory
    install(DIRECTORY sddm-theme/ DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze PATTERN "README.txt" EXCLUDE PATTERN "components" EXCLUDE PATTERN "dummydata" EXCLUDE
    PATTERN "theme.conf.cmake" EXCLUDE)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sddm-theme/theme.conf DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze)
    install(DIRECTORY lookandfeel/contents/components DESTINATION ${KDE_INSTALL_FULL_DATADIR}/sddm/themes/breeze PATTERN "README.txt" EXCLUDE)

endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# locate qdbus in the Qt path because not every distro makes a symlink at /usr/bin/qdbus
query_qmake(QtBinariesDir QT_INSTALL_BINS)

add_subdirectory(doc)
add_subdirectory(libkworkspace)
add_subdirectory(libdbusmenuqt)
add_subdirectory(appmenu)

add_subdirectory(libtaskmanager)
add_subdirectory(libnotificationmanager)
add_subdirectory(libcolorcorrect)
add_subdirectory(components)

add_subdirectory(plasma-windowed)
add_subdirectory(shell)
add_subdirectory(freespacenotifier)
add_subdirectory(klipper)
add_subdirectory(krunner)
add_subdirectory(ksmserver)
add_subdirectory(logout-greeter)
add_subdirectory(ksplash)
add_subdirectory(systemmonitor)
add_subdirectory(statusnotifierwatcher)
add_subdirectory(startkde)
add_subdirectory(themes)

add_subdirectory(kcms)

add_subdirectory(containmentactions)
add_subdirectory(runners)
add_subdirectory(applets)
add_subdirectory(dataengines)
add_subdirectory(wallpapers)

add_subdirectory(kioslave)
add_subdirectory(ktimezoned)
add_subdirectory(menu)
add_subdirectory(phonon)

# This ensures pressing the eject button on a CD drive ejects the disc
# It listens to the Solid::OpticalDrive::ejectPressed signal that is only
# supported by Solid in the HAL backend and does nothing with UDev
if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
add_subdirectory(solidautoeject)
endif()

ecm_optional_add_subdirectory(xembed-sni-proxy)

ecm_optional_add_subdirectory(gmenu-dbusmenu-proxy)

add_subdirectory(soliduiserver)

if(KF5Holidays_FOUND)
    add_subdirectory(plasmacalendarintegration)
endif()

add_subdirectory(templates)

install(FILES plasma-workspace.categories   DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
