# SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.mediacontroller\")

set(mediacontrollerplugin_SRCS
    mediacontrollerplugin.cpp
    globalconfig.cpp
    playerhistorymodel.cpp
)

add_library(mediacontrollerplugin SHARED ${mediacontrollerplugin_SRCS})
target_link_libraries(mediacontrollerplugin
    PRIVATE
        Qt::Core
        Qt::Concurrent
        Qt::Qml
        KF6::ConfigCore
        KF6::I18n
)

install(TARGETS mediacontrollerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/mediacontroller)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/mediacontroller)
