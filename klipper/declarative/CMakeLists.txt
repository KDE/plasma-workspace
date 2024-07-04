# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
ecm_add_qml_module(klipperplugin URI org.kde.plasma.private.clipboard VERSION 0.1 GENERATE_PLUGIN_SOURCE)

target_sources(klipperplugin
    PRIVATE
        declarativehistorymodel.cpp
)
target_link_libraries(klipperplugin
    PRIVATE
        Qt::Qml
        klipper
)

ecm_finalize_qml_module(klipperplugin)