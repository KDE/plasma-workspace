# SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(batterycontrol_SRCS
        batterycontrol.cpp batterycontrol.h
        batteriesnamesmonitor_p.cpp batteriesnamesmonitor_p.h
)

add_definitions(-DTRANSLATION_DOMAIN=\"batterycontrol\")

ecm_qt_declare_logging_category(batterycontrol_SRCS
        HEADER "batterycontrol_debug.h"
        IDENTIFIER "COMPONENTS::BATTERYCONTROL"
        CATEGORY_NAME org.kde.batterycontrol
        DEFAULT_SEVERITY Warning
        DESCRIPTION "Battery List Model" EXPORT "COMPONENTS::BATTERYCONTROL"
)

add_library(batterycontrol SHARED ${batterycontrol_SRCS})

generate_export_header(batterycontrol)

target_include_directories(batterycontrol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(batterycontrol
        PUBLIC
        Qt::Core
        Qt::Qml
        KF6::Solid
        PRIVATE
        Qt::DBus
        KF6::Service
        KF6::I18n
)

set_target_properties(batterycontrol PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

ecm_add_qml_module(batterycontrol URI org.kde.plasma.private.battery GENERATE_PLUGIN_SOURCE)
ecm_finalize_qml_module(batterycontrol)

install(TARGETS batterycontrol ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_qt_install_logging_categories(
        EXPORT "COMPONENTS::BATTERYCONTROL"
        FILE components/batterycontrol.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)