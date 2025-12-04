# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

plasma_add_applet(org.kde.plasma.cameraindicator
    QML_SOURCES
        qml/main.qml
        qml/FullRepresentation.qml
    GENERATE_APPLET_CLASS
)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
