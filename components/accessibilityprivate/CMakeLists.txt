# SPDX-FileCopyrightText: 2026 Martin Riethmayer <ripper@freakmail.de>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_qml_module(accessibilityprivateplugin
    URI "org.kde.plasma.private.accessibility"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtCore
)

target_sources(accessibilityprivateplugin PRIVATE
    accessibilitystatus.cpp
)

target_link_libraries(accessibilityprivateplugin PRIVATE
    Qt::Core
    Qt::Qml
    KF6::ConfigCore
)

ecm_finalize_qml_module(accessibilityprivateplugin)
