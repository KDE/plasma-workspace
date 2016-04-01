cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(plasma-%{APPNAMEID})

set(QT_MIN_VERSION "5.5.0")
set(KF5_MIN_VERSION "5.18.0")

find_package(ECM 1.8.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
    COMPONENTS
        Gui
        Network
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    COMPONENTS
        Plasma
        UnitConversion
)

find_package(PlasmaWeatherIon REQUIRED)
set_package_properties(PlasmaWeatherIon PROPERTIES
    DESCRIPTION "Plasma Weather Ion library"
    TYPE REQUIRED
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

add_subdirectory(src)
