cmake_minimum_required(VERSION 3.16)

project(plasma-workspace)
set(PROJECT_VERSION "5.90.90")
string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_LIST 1 PROJECT_VERSION_MINOR)
list(GET VERSION_LIST 2 PROJECT_VERSION_PATCH)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "5.240.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set(INSTALL_SDDM_THEME TRUE)
option(PLASMA_X11_DEFAULT_SESSION "Use X11 session by default for Plasma" OFF)
option(INSTALL_SDDM_WAYLAND_SESSION OFF)
option(WITH_X11 "Build with X11 support. Building without is experimental" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
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

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Svg Widgets Quick QuickWidgets Concurrent Network Core5Compat DBus ShaderTools)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
                    Auth Parts Runner Notifications NotifyConfig NewStuff Wallet IdleTime
                    Svg Declarative I18n KCMUtils TextWidgets Crash GlobalAccel DBusAddons
                    CoreAddons KIO Prison Package
                    GuiAddons Archive ItemModels IconThemes UnitConversion TextEditor StatusNotifierItem
                    OPTIONAL_COMPONENTS DocTools)
find_package(KF6 ${PROJECT_VERSION} REQUIRED COMPONENTS Plasma5Support)
find_package(Plasma ${PROJECT_VERSION} REQUIRED)
find_package(PlasmaQuick ${PROJECT_VERSION} REQUIRED)
find_package(KWayland ${PROJECT_VERSION} REQUIRED)
find_package(PlasmaActivities ${PROJECT_VERSION} REQUIRED)
find_package(PlasmaActivitiesStats ${PROJECT_VERSION} REQUIRED)

if (BUILD_TESTING)
    include(ECMAddTests)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test QuickTest)
endif()

find_package(KF6KDED CONFIG REQUIRED)
find_package(KPipeWire CONFIG)

find_package(KF6NetworkManagerQt ${KF6_MIN_VERSION})
set_package_properties(KF6NetworkManagerQt PROPERTIES DESCRIPTION "Qt wrapper for NetworkManager API"
                       TYPE OPTIONAL
                       PURPOSE "Needed by geolocation data engine."
                      )

find_package(KF6Kirigami2 ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6Kirigami2 PROPERTIES
    DESCRIPTION "A QtQuick based components set"
    TYPE RUNTIME
)

find_package(KF6KirigamiAddons2 0.10.0 CONFIG)
set_package_properties(KF6KirigamiAddons2 PROPERTIES
    DESCRIPTION "Extra controls for Kirigami applications"
    PURPOSE "Required at runtime for the User KCM"
    TYPE RUNTIME
)

find_package(KF6QuickCharts ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6QuickCharts PROPERTIES
    DESCRIPTION "Used for rendering charts"
    TYPE RUNTIME
)

find_package(KF6UserFeedback)
find_package(KSysGuard CONFIG REQUIRED)

find_package(KF6Baloo)
set_package_properties(KF6Baloo PROPERTIES DESCRIPTION "File Searching"
                       TYPE RECOMMENDED
                       PURPOSE "Needed for the File Search runner."
                      )

find_package(PkgConfig REQUIRED)
pkg_check_modules(QALCULATE libqalculate>2.0 REQUIRED IMPORTED_TARGET)

find_package(KWinDBusInterface CONFIG REQUIRED)

find_package(KF6Screen CONFIG REQUIRED)
find_package(KScreenLocker 5.13.80 REQUIRED)
find_package(ScreenSaverDBusInterface CONFIG REQUIRED)
find_package(LayerShellQt CONFIG REQUIRED)
find_package(KF6Holidays)
set_package_properties(KF6Holidays PROPERTIES DESCRIPTION "Holidays provider for Plasma calendar"
                       TYPE OPTIONAL
                       PURPOSE "Needed to for holidays plugin for Plasma Calendar."
                      )
# Used in kcm_autostart
pkg_check_modules(SYSTEMD "systemd")

find_package(Phonon4Qt6 4.6.60 REQUIRED NO_MODULE)
set_package_properties(Phonon4Qt6 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE REQUIRED)

find_package(Canberra)
set_package_properties(Canberra PROPERTIES
    PURPOSE "Needed to preview notification sounds"
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

if(PLASMA_X11_DEFAULT_SESSION)
    set(WITH_X11 1)
endif()

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

else()
    set(HAVE_X11 0)
    set(HAVE_XCURSOR 0)
    set(HAVE_XFIXES 0)
endif()

pkg_check_modules(PipeWire IMPORTED_TARGET libpipewire-0.3)
add_feature_info(PipeWire PipeWire_FOUND "Required for Wayland screencasting")

if(PipeWire_FOUND)
  find_package(Libdrm REQUIRED)
endif()

find_package(Qt6 REQUIRED CONFIG COMPONENTS WaylandCompositor)
find_package(Qt6WaylandClient COMPONENTS Private)
find_package(PlasmaWaylandProtocols 1.6 REQUIRED)
find_package(Wayland REQUIRED COMPONENTS Client Server) # Server is used in autotests

if(FONTCONFIG_FOUND)
  # kfontinst
  find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS PrintSupport)
endif()

find_package(AppStreamQt 1.0) # 1.0 will be the first release to support Qt6
set_package_properties(AppStreamQt PROPERTIES DESCRIPTION "Access metadata for listing available software"
                       URL "https://www.freedesktop.org/wiki/Distributions/AppStream/"
                       TYPE OPTIONAL)

if(${AppStreamQt_FOUND})
    set(HAVE_APPSTREAMQT true)
endif()

find_package(UDev REQUIRED)
set_package_properties(UDev PROPERTIES
    PURPOSE "Allows to play notification sounds when (un)plugging devices"
    TYPE OPTIONAL
)

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
    # You should enable GLIBC_LOCALE_PREGENERATED option
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
        find_program(LANGUAGE_SELECTOR_COMMON "check-language-support" DOC "Required for ubuntu-based distros to generate locale")
    endif()

    set(UBUNTU_LOCALE FALSE)
    option(UBUNTU_PACKAGEKIT "Install required package for language on Ubuntu Based systems, PackageKitQt6 required" ${SYSTEM_UBUNTU_BASED})
    if(UBUNTU_PACKAGEKIT)
        find_package(PackageKitQt6)
        set_package_properties(PackageKitQt6
                PROPERTIES DESCRIPTION "Software Manager integration"
                TYPE OPTIONAL
                PURPOSE "Used to install additional language packages on demand"
                )
        set(UBUNTU_LOCALE ${PackageKitQt6_FOUND})
    endif()

    set(GLIBC_LOCALE_GEN_DEFAULT ON)
    if(UBUNTU_LOCALE)
        set(GLIBC_LOCALE_GEN_DEFAULT OFF)
    endif()

    option(GLIBC_LOCALE_GEN "Auto generate Glibc locale with locale-gen and /etc/locale.gen" ${GLIBC_LOCALE_GEN_DEFAULT})
    if(GLIBC_LOCALE_GEN AND UBUNTU_LOCALE)
        message(FATAL_ERROR "UBUNTU_PACKAGEKIT and GLIBC_LOCALE_GEN both enabled, only UBUNTU_PACKEGKIT will be used")
    endif()
    if(GLIBC_LOCALE_GEN)
        set(GLIBC_LOCALE_AUTO TRUE)
    endif()
    set(REGION_LANG_GENERATE_LOCALE_HELPER FALSE)
    if(UBUNTU_LOCALE OR GLIBC_LOCALE_GEN)
        set(REGION_LANG_GENERATE_LOCALE_HELPER TRUE)
    endif()

    option(GLIBC_LOCALE_PREGENERATED "Systems that don't require locale generation. Such as openSUSE or Fedora" OFF)
    if(GLIBC_LOCALE_PREGENERATED)
        set(GLIBC_LOCALE_GENERATED TRUE)
    endif()
    if(REGION_LANG_GENERATE_LOCALE_HELPER AND GLIBC_LOCALE_PREGENERATED)
        message(FATAL_ERROR "(UBUNTU_PACKAGEKIT || GLIBC_LOCALE_GEN) and GLIBC_LOCALE_PREGENERATED both enabled")
    endif()

    option(GLIBC_LOCALE "Build with glibc-compatible locale infrastructure" ON)
    add_feature_info(GLIBC_LOCALE GLIBC_LOCALE "Locale support is glibc based")
    add_feature_info(UBUNTU_LOCALE UBUNTU_LOCALE "Install language packages on Ubuntu-like systems")
    add_feature_info(GLIBC_LOCALE_GEN GLIBC_LOCALE_GEN "Generate glibc locales")
    add_feature_info(GLIBC_LOCALE_PREGENERATED GLIBC_LOCALE_PREGENERATED "glibc locales are pregenerated")
    ################## Find libraries ###################
    if(REGION_LANG_GENERATE_LOCALE_HELPER)
        find_package(PolkitQt6-1)
        set_package_properties(PolkitQt6-1
                PROPERTIES DESCRIPTION "DBus interface wrapper for Polkit"
                PURPOSE "Communicate with localegen helper in region & lang kcm"
                TYPE REQUIRED
        )
    endif()
endif()

find_package(ICU COMPONENTS i18n uc)
set_package_properties(ICU
        PROPERTIES DESCRIPTION "Unicode and Globalization support for software applications"
        TYPE REQUIRED
        PURPOSE "Better application groups in Kicker, timezone support"
        )
if(${ICU_FOUND})
    set(HAVE_ICU TRUE)
endif()

find_package(KExiv2Qt6)
set_package_properties(KExiv2Qt6
        PROPERTIES URL "https://commits.kde.org/libkexiv2"
        DESCRIPTION "Image metadata support"
        TYPE OPTIONAL
        PURPOSE "Provides metadata for image wallpaper plugin"
        )
if(${KExiv2Qt6_FOUND})
    set(HAVE_KExiv2 TRUE)
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

if(Qt6WaylandClient_VERSION VERSION_LESS "6.6.0")
    pkg_check_modules(XKBCOMMON xkbcommon REQUIRED IMPORTED_TARGET)
endif()

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
    KRUNNER 5.103.0
)

configure_file(config-workspace.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-workspace.h)
configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)
configure_file(config-appstream.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-appstream.h )
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

if(KF6DocTools_FOUND)
  add_subdirectory(doc)
  kdoctools_install(po)
endif()
add_subdirectory(libkworkspace)
add_subdirectory(libdbusmenuqt)
add_subdirectory(appmenu)

add_subdirectory(libtaskmanager)
add_subdirectory(libkmpris)
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

if(KF6Holidays_FOUND)
    add_subdirectory(plasmacalendarintegration)
endif()

add_subdirectory(appiumtests)

if (TARGET UDev::UDev)
    add_subdirectory(devicenotifications)
endif()

ki18n_install(po)

install(FILES completions/plasmashell.zsh RENAME _plasmashell DESTINATION ${KDE_INSTALL_ZSHAUTOCOMPLETEDIR})

ecm_qt_install_logging_categories(
        EXPORT PLASMAWORKSPACE
        FILE plasma-workspace.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
