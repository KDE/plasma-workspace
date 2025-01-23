# SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(timezoneplugin
    URI "org.kde.plasma.workspace.timezoneselector"
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES
        org.kde.kirigami
)

target_sources(timezoneplugin PRIVATE
    timezoneutils.h
    timezoneutils.cpp
)

ecm_target_qml_sources(timezoneplugin SOURCES
    TimezoneSelector.qml
    GeoJsonDelegate.qml
)

target_link_libraries(timezoneplugin PRIVATE
    Qt6::Quick
    Qt6::Qml
    KF6::ConfigCore
    Qt6::QuickControls2
)

ecm_finalize_qml_module(timezoneplugin)

install(FILES timezones.json DESTINATION ${KDE_INSTALL_DATADIR}/timezonefiles)

qt_add_resources(
    timezoneplugin "timezone_files"
    PREFIX "plasma-workspace/timezone"
    FILES "offline_tiles/"
)
