# SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

plasma_add_applet(org.kde.plasma.analogclock
    QML_SOURCES
        qml/main.qml
        qml/Hand.qml
        qml/configGeneral.qml
        qml/config.qml
    RESOURCES
        main.xml
    GENERATE_APPLET_CLASS
)
