# SPDX-License-Identifier: BSD-2-Clause
# 
# SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

ecm_add_qml_module(loginlockscreen
    URI "org.kde.plasma.workspace.loginlockscreen"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

ecm_target_qml_sources(loginlockscreen
    SOURCES
    Footer.qml
)

ecm_finalize_qml_module(loginlockscreen)
