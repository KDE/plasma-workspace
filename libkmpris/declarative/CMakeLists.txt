# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(libmprisplugin_SRCS
    libmprisplugin.cpp
)

add_library(mprisplugin SHARED ${libmprisplugin_SRCS})
target_link_libraries(mprisplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        kmpris
)

install(TARGETS mprisplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/mpris)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/mpris)

