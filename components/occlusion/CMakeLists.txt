# SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(occlusionplugin
    URI "org.kde.plasma.workspace.occlusion"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

target_sources(occlusionplugin PRIVATE
    occlusion.cpp
)

target_link_libraries(occlusionplugin PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    PW::LibTaskManager
)

ecm_finalize_qml_module(occlusionplugin)
