#CMakeLists.txt
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_regionandlang\")
################## Set sources files #################
set(kcm_regionandlang_example_SRCS
    exampleutility.cpp
)
add_library(kcm_regionandlang_example_static STATIC ${kcm_regionandlang_example_SRCS})
set_property(TARGET kcm_regionandlang_example_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kcm_regionandlang_example_static
    KF5::I18n
    KF5::CoreAddons
)
set(kcm_regionandlang_PART_SRCS
    settingtype.h
    kcmregionandlang.cpp
    localelistmodel.cpp
    optionsmodel.cpp
    languagelistmodel.cpp
    localegenerator.cpp
    localegeneratorbase.cpp
    localegeneratorgeneratedglibc.cpp
    regionandlangsettings.cpp
)
kconfig_add_kcfg_files(kcm_regionandlang_PART_SRCS regionandlangsettingsbase.kcfgc GENERATE_MOC)

if(GLIBC_LOCALE_GEN)
    set(kcm_regionandlang_PART_SRCS
        ${kcm_regionandlang_PART_SRCS}
        localegeneratorglibc.cpp)
endif()
if(UBUNTU_PACKAGEKIT)
    set(kcm_regionandlang_PART_SRCS
        ${kcm_regionandlang_PART_SRCS}
        localegeneratorubuntu.cpp)
endif()
ecm_qt_declare_logging_category(
    kcm_regionandlang_PART_SRCS
    HEADER kcm_regionandlang_debug.h
    IDENTIFIER KCM_REGIONANDLANG
    CATEGORY_NAME org.kde.kcm_regionandlang
    DESCRIPTION "Region and Language KCM"
    EXPORT kcm_regionandlang
)

################ Build helper and add additional source files #############
if(REGION_LANG_GENERATE_LOCALE)
    add_subdirectory("localegenhelper")
    qt_generate_dbus_interface(
        ${CMAKE_CURRENT_SOURCE_DIR}/localegenhelper/localegenhelper.h
        org.kde.localegenhelper.xml
        OPTIONS -s -m
    )
    qt_add_dbus_interface(kcm_regionandlang_PART_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.localegenhelper.xml localegenhelperinterface)
endif()

#################### Declare target #######################
kcoreaddons_add_plugin(kcm_regionandlang SOURCES ${kcm_regionandlang_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")

kcmutils_generate_desktop_file(kcm_regionandlang)
ecm_qt_install_logging_categories(
    EXPORT kcm_regionandlang
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

################# Link libraries ####################
target_link_libraries(kcm_regionandlang
    kcm_regionandlang_example_static
    Qt::Core
    Qt::DBus
    KF5::I18n
    KF5::KCMUtils
    KF5::QuickAddons
    KF5::ItemModels)
if (QT_MAJOR_VERSION EQUAL "6")
    target_link_libraries(kcm_regionandlang Qt::Core5Compat) # for QTextCodec
endif()
if(UBUNTU_PACKAGEKIT)
    target_link_libraries(kcm_regionandlang PK::packagekitqt${QT_MAJOR_VERSION})
endif()
########### install files ###############
kpackage_install_package(package kcm_regionandlang kcms)

########### unit tests ##############
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
