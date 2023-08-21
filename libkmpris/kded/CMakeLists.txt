# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

kcoreaddons_add_plugin(mprisservice SOURCES mprisservice.cpp INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(mprisservice
    Qt::Core
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::XmlGui
    kmpris
)
