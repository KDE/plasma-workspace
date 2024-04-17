# SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(brightnesscontrolplugin URI org.kde.plasma.private.brightnesscontrolplugin GENERATE_PLUGIN_SOURCE)
target_sources(brightnesscontrolplugin PRIVATE
    nightlightinhibitor.cpp
    nightlightcontrol.cpp
    screenbrightnesscontrol.cpp
    keyboardbrightnesscontrol.cpp
    keyboardcolorcontrol.cpp
)
target_link_libraries(brightnesscontrolplugin PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Qml
    QCoro::Coro
    QCoro::DBus
)
ecm_finalize_qml_module(brightnesscontrolplugin)
