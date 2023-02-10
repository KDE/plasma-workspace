cmake_minimum_required(VERSION 3.16)

project(plasma-workspace)
set(PROJECT_VERSION "5.27.0")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.102.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set(INSTALL_SDDM_THEME TRUE)
option(INSTALL_SDDM_WAYLAND_SESSION OFF)
option(WITH_X11 "Build with X11 support. Building without is experimental" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
include(ECMQueryQt)
include(ECMInstallIcons)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMConfiguredInstall)
include(ECMGenerateDBusServiceFile)
include(ECMFindQmlModule)
include(ECMGenerateExportHeader)
include(ECMGenerateQmlTypes)
include(ECMDeprecationSettings)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Svg Widgets Quick QuickWidgets Concurrent Test Network)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
                    Auth Plasma Runner Notifications NotifyConfig NewStuff Wallet IdleTime
                    Declarative I18n KCMUtils TextWidgets Crash GlobalAccel DBusAddons Wayland
                    CoreAddons People ActivitiesStats Activities KIO Prison PlasmaQuick Package
                    GuiAddons Archive ItemModels IconThemes UnitConversion ItemModels TextEditor
                    OPTIONAL_COMPONENTS DocTools)

find_package(KDED CONFIG REQUIRED)
find_package(KPipeWire CONFIG)

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

find_package(KUserFeedback)
find_package(KSysGuard CONFIG REQUIRED)

find_package(KF5Baloo)
set_package_properties(KF5Baloo PROPERTIES DESCRIPTION "File Searching"
                       TYPE RECOMMENDED
                       PURPOSE "Needed for the File Search runner."
                      )

pkg_check_modules(QALCULATE libqalculate>2.0 REQUIRED IMPORTED_TARGET)

find_package(KWinDBusInterface CONFIG REQUIRED)

find_package(KF5Screen CONFIG REQUIRED)
find_package(KScreenLocker 5.13.80 REQUIRED)
find_package(ScreenSaverDBusInterface CONFIG REQUIRED)
find_package(LayerShellQt CONFIG REQUIRED)
find_package(KF5Holidays)
set_package_properties(KF5Holidays PROPERTIES DESCRIPTION "Holidays provider for Plasma calendar"
                       TYPE OPTIONAL
                       PURPOSE "Needed to for holidays plugin for Plasma Calendar."
                      )

find_package(Phonon4Qt${QT_MAJOR_VERSION} 4.6.60 REQUIRED NO_MODULE)
set_package_properties(Phonon4Qt${QT_MAJOR_VERSION} PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE REQUIRED)

find_package(Breeze ${PROJECT_VERSION} CONFIG)
set_package_properties(Breeze PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "For setting the default window decoration plugin")

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "https://www.zlib.net"
                       TYPE REQUIRED
                      )

find_package(Fontconfig)
set_package_properties(Fontconfig PROPERTIES DESCRIPTION "Font access configuration library"
                       URL "https://www.freedesktop.org/wiki/Software/fontconfig"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build font configuration and installation tools"
                      )

if(WITH_X11)
    find_package(X11)
    set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                            URL "https://www.x.org"
                            TYPE REQUIRED
                            PURPOSE "Required for building the X11 based workspace")
    function(check_X11_lib _lib)
        if (NOT TARGET X11::${_lib})
            message(SEND_ERROR "Required component ${_lib} of X11 was not found")
        endif()
    endfunction()
    check_X11_lib(ICE)
    check_X11_lib(SM)
    check_X11_lib(X11)
    check_X11_lib(Xau)
    check_X11_lib(Xcursor)
    check_X11_lib(Xfixes)
    check_X11_lib(Xft)
    check_X11_lib(Xrender)
    check_X11_lib(Xtst)

    set(HAVE_X11 1)
    set(HAVE_XCURSOR 1)
    set(HAVE_XFIXES 1)

    find_package(XCB MODULE REQUIRED COMPONENTS XCB RANDR IMAGE)
    set_package_properties(XCB PROPERTIES TYPE REQUIRED)

    if (QT_MAJOR_VERSION EQUAL "5")
        find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS X11Extras)
    endif()

else()
    set(HAVE_X11 0)
    set(HAVE_XCURSOR 0)
    set(HAVE_XFIXES 0)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(PipeWire IMPORTED_TARGET libpipewire-0.3)
add_feature_info(PipeWire PipeWire_FOUND "Required for Wayland screencasting")

if(PipeWire_FOUND)
  find_package(Libdrm REQUIRED)
endif()

if(QT_MAJOR_VERSION EQUAL "5")
  find_package(QtWaylandScanner REQUIRED)
  find_package(Qt5XkbCommonSupport)
else()
  find_package(Qt6 REQUIRED CONFIG COMPONENTS WaylandCompositor)
endif()
find_package(Qt${QT_MAJOR_VERSION}WaylandClient COMPONENTS Private)
find_package(PlasmaWaylandProtocols 1.6 REQUIRED)
find_package(Wayland REQUIRED COMPONENTS Client Server) # Server is used in autotests
find_package(WaylandProtocols 1.31)
set_package_properties(WaylandProtocols PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Collection of Wayland protocols that add functionality not available in the Wayland core protocol"
    URL "https://gitlab.freedesktop.org/wayland/wayland-protocols/"
)
if (WaylandProtocols_FOUND)
    set(HAVE_WaylandProtocols true)
endif()

if(FONTCONFIG_FOUND)
  # kfontinst
  find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS PrintSupport)
endif()

find_package(AppStreamQt 0.10.6)
set_package_properties(AppStreamQt PROPERTIES DESCRIPTION "Access metadata for listing available software"
                       URL "https://www.freedesktop.org/wiki/Distributions/AppStream/"
                       TYPE OPTIONAL)

if(${AppStreamQt_FOUND})
    set(HAVE_APPSTREAMQT true)
endif()


# Region & Language KCM
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # notes for packager:
    # This is the macro definition part of Region & Language KCM
    # Depending on your distributions, the KCM behaves differently
    # For Ubuntu based systems, this KCM uses "check-language-support" and PackageKit
    # to install relevant fonts and packages
    # You should enable option UBUNTU_PACKAGEKIT (it's auto detected though)
    #
    # For Glibc systems that don't come with pre-generated locales, such as ArchLinux
    # This KCM uses "/etc/locale.gen" and "locale-gen" to generate configured locales
    # and display a note to let user install fonts themselves if required
    # You shouldn't required to do anything in this case
    #
    # For Glibc systems that come with pre-generated locales, such as Fedora and openSUSE
    # a note to let user install fonts themselves if required is displayed
    # You should enable GLIBC_LOCALE_GENERATED option
    #
    # For non-glibc systems such as VoidLinux and *BSD
    # A warning of configure locale manually is displayed (although the relevant ENVs are set by Plasma)
    # You should disable GLIBC_LOCALE_GEN option


    # find out if the build system is Ubuntu based
    if (${CMAKE_VERSION} VERSION_LESS 3.22)
        find_program(LSB_RELEASE_EXEC lsb_release)
        execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
            OUTPUT_VARIABLE DISTRO_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(TOLOWER "${DISTRO_NAME}" DISTRO_NAME)
    else()
        cmake_host_system_information(RESULT DISTRO_NAME QUERY DISTRIB_ID)
        cmake_host_system_information(RESULT DISTRO_ID_LIKE QUERY DISTRIB_ID_LIKE)
    endif()

    string(COMPARE EQUAL "ubuntu" "${DISTRO_NAME}" SYSTEM_UBUNTU_BASED)
    string(FIND "${DISTRO_ID_LIKE}" "ubuntu" FINDINDEX)
    if(NOT (FINDINDEX EQUAL -1))
        set(SYSTEM_UBUNTU_BASED ON)
    endif()

    option(UBUNTU_PACKAGEKIT "Install required package for language on Ubuntu Based systems, PackageKitQt5 required" ${SYSTEM_UBUNTU_BASED})
    if(UBUNTU_PACKAGEKIT)
        set(OS_UBUNTU TRUE)
    endif()

    set(GLIBC_LOCALE_GEN_DEFAULT ON)
    if(UBUNTU_PACKAGEKIT)
        set(GLIBC_LOCALE_GEN_DEFAULT OFF)
    endif()

    option(GLIBC_LOCALE_GEN "Auto generate Glibc locale with locale-gen and /etc/locale.gen" ${GLIBC_LOCALE_GEN_DEFAULT})
    if(GLIBC_LOCALE_GEN AND UBUNTU_PACKAGEKIT)
        message(FATAL_ERROR "UBUNTU_PACKAGEKIT and GLIBC_LOCALE_GEN both enabled, only UBUNTU_PACKEGKIT will be used")
    endif()
    if(GLIBC_LOCALE_GEN)
        set(GLIBC_LOCALE TRUE)
    endif()
    if(UBUNTU_PACKAGEKIT OR GLIBC_LOCALE_GEN)
        set(REGION_LANG_GENERATE_LOCALE TRUE)
    endif()

    option(GLIBC_LOCALE_GENERATED "Systems that don't require locale generation. Such as openSUSE or Fedora" OFF)
    if(GLIBC_LOCALE_GENERATED)
        set(LOCALE_GENERATED TRUE)
    endif()
    if(REGION_LANG_GENERATE_LOCALE AND GLIBC_LOCALE_GENERATED)
        message(FATAL_ERROR "(UBUNTU_PACKAGEKIT || GLIBC_LOCALE_GEN) and GLIBC_LOCALE_GENERATED both enabled")
    endif()

    ################## Find libraries ###################
    if(REGION_LANG_GENERATE_LOCALE)
        find_package(PolkitQt${QT_MAJOR_VERSION}-1)
        set_package_properties(PolkitQt${QT_MAJOR_VERSION}-1
                PROPERTIES DESCRIPTION "DBus interface wrapper for Polkit"
                PURPOSE "Communicate with localegen helper in region & lang kcm"
                TYPE REQUIRED
        )
    endif()

    if(UBUNTU_PACKAGEKIT)
        find_package(PackageKitQt${QT_MAJOR_VERSION})
        set_package_properties(PackageKitQt${QT_MAJOR_VERSION}
                PROPERTIES DESCRIPTION "Software Manager integration"
                TYPE OPTIONAL
                PURPOSE "Used to install additional language packages on demand"
                )

    endif()
endif()

find_package(ICU COMPONENTS i18n uc)
set_package_properties(ICU
        PROPERTIES DESCRIPTION "Unicode and Globalization support for software applications"
        TYPE OPTIONAL
        PURPOSE "Better application groups in Kicker"
        )
if(${ICU_FOUND})
    set(HAVE_ICU TRUE)
endif()

find_package(KF5KExiv2)
set_package_properties(KF5KExiv2
        PROPERTIES URL "https://www.exiv2.org" DESCRIPTION "Image metadata support"
        TYPE OPTIONAL
        PURPOSE "Provides metadata for image wallpaper plugin"
        )
if(${KF5KExiv2_FOUND})
    set(HAVE_KF5KExiv2 TRUE)
endif()

find_package(KIOExtras)
set_package_properties(KIOExtras PROPERTIES DESCRIPTION "Common KIO slaves for operations."
                                       PURPOSE "Show thumbnails in wallpaper selection."
                                       TYPE RUNTIME
                                        )

find_package(KIOFuse)
set_package_properties(KIOFuse PROPERTIES DESCRIPTION "Provide KIO support to legacy applications. "
                                      TYPE RUNTIME
                                      )

# Clipboard applet
ecm_find_qmlmodule(org.kde.prison 1.0)

# Is building in KDE CI
if(DEFINED ENV{KDECI_BUILD})
    set(KDECI_BUILD true)
endif()

include(ConfigureChecks.cmake)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 5.98.0
    KDECLARATIVE 5.88.0
)

configure_file(config-workspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-workspace.h)
configure_file(config-unix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-unix.h )
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)
configure_file(config-appstream.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-appstream.h )
configure_file(config-WaylandProtocols.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-WaylandProtocols.h )
configure_file(config-KDECI_BUILD.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-KDECI_BUILD.h )
add_subdirectory(login-sessions)
add_subdirectory(lookandfeel)

if (INSTALL_SDDM_WAYLAND_SESSION)
    install(FILES sddm-wayland-session/plasma-wayland.conf DESTINATION /etc/sddm.conf.d)
else()
    message(STATUS "INSTALL_SDDM_WAYLAND_SESSION is disabled. As soon as it's installed, SDDM will default to use Wayland and KWin for its greeter session (BETA, do not deploy to final users yet).")
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

# locate qdbus in the Qt path because not every distro makes a symlink at /usr/bin/qdbus
ecm_query_qt(QtBinariesDir QT_INSTALL_BINS)

option(PLASMA_WAYLAND_DEFAULT_SESSION "Use Wayland session by default for Plasma" FALSE)

if(KF5DocTools_FOUND)
  add_subdirectory(doc)
  kdoctools_install(po)
endif()
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

add_subdirectory(kioworkers)
add_subdirectory(ktimezoned)
add_subdirectory(menu)
add_subdirectory(phonon)

add_subdirectory(interactiveconsole)

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

add_subdirectory(appiumtests)

ki18n_install(po)

install(FILES completions/plasmashell.zsh RENAME _plasmashell DESTINATION ${KDE_INSTALL_ZSHAUTOCOMPLETEDIR})

install(FILES plasma-workspace.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
