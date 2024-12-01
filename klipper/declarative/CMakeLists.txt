# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(klipperplugin
    URI org.kde.plasma.private.clipboard
    VERSION 0.1
    CLASS_NAME KlipperPlugin
    DEPENDENCIES
        "QtQuick"
        "QtQuick.Templates"
        "Qt.labs.qmlmodels"
        "org.kde.kirigami"
        "org.kde.kitemmodels"
        "org.kde.kquickcontrolsaddons"
        "org.kde.ksvg"
        "org.kde.plasma.components"
        "org.kde.plasma.extras"
        "org.kde.prison"
    NO_GENERATE_PLUGIN_SOURCE
)

target_sources(klipperplugin
    PRIVATE
        klipperplugin.cpp
        previewimageprovider.cpp previewimageprovider.h
        declarativehistorymodel.cpp declarativehistorymodel.h
        klipperinterface.cpp klipperinterface.h
)
target_link_libraries(klipperplugin
    PRIVATE
        Qt::Qml
        klipper
)

ecm_target_qml_sources(klipperplugin VERSION 0.1 SOURCES
    qml/KlipperPopup.qml
    qml/ClipboardMenu.qml
    qml/ClipboardItemDelegate.qml
    qml/TextItemDelegate.qml
    qml/UrlItemDelegate.qml
    qml/ImageItemDelegate.qml
    qml/BarcodePage.qml
    qml/DelegateToolButtons.qml
    qml/EditPage.qml
)

ecm_finalize_qml_module(klipperplugin)