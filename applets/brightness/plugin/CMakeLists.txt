# SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(brightnesscontrolplugin URI org.kde.plasma.private.brightnesscontrolplugin)
target_sources(brightnesscontrolplugin PRIVATE
    nightcolorinhibitor.cpp
    nightcolormonitor.cpp
    screenbrightnesscontrol.cpp
    keyboardbrightnesscontrol.cpp
    plugin.cpp
)
target_link_libraries(brightnesscontrolplugin PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Qml
)
ecm_finalize_qml_module(brightnesscontrolplugin)
