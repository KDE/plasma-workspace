# SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

plasma_add_applet(org.kde.plasma.calendar
    QML_SOURCES
        qml/main.qml
        qml/config.qml
        qml/configGeneral.qml
    CPP_SOURCES
        calendarapplet.cpp
    RESOURCES
        main.xml
        mini-calendar.svgz
)

target_link_libraries(org.kde.plasma.calendar PRIVATE Plasma::Plasma)
