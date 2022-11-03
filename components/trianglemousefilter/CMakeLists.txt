# SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

set(trianglemousefilterplugin_SRCS
    trianglemousefilterplugin.cpp
    trianglemousefilter.cpp
)

ecm_qt_declare_logging_category(trianglemousefilterplugin_SRCS HEADER debug.h
    IDENTIFIER TRIANGLE_MOUSE_FILTER
    CATEGORY_NAME kde.trianglemousefilter
    DEFAULT_SEVERITY Info)

add_library(trianglemousefilterplugin SHARED ${trianglemousefilterplugin_SRCS})

target_link_libraries(trianglemousefilterplugin Qt::Core
                                           Qt::Quick
                                           Qt::Qml)

set(trianglemousefilterplugin_PATH /org/kde/plasma/workspace/trianglemousefilter)
install(TARGETS trianglemousefilterplugin
  DESTINATION ${KDE_INSTALL_QMLDIR}${trianglemousefilterplugin_PATH})
install(FILES qmldir
  DESTINATION ${KDE_INSTALL_QMLDIR}${trianglemousefilterplugin_PATH})
