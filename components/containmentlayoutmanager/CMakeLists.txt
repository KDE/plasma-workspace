ecm_add_qml_module(containmentlayoutmanagerplugin
    URI "org.kde.plasma.private.containmentlayoutmanager"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES QtQuick
)

target_sources(containmentlayoutmanagerplugin PRIVATE
    appletcontainer.cpp
    configoverlay.cpp
    appletslayout.cpp
    abstractlayoutmanager.cpp
    gridlayoutmanager.cpp
    itemcontainer.cpp
    resizehandle.cpp
)

ecm_target_qml_sources(containmentlayoutmanagerplugin
    SOURCES
    qml/BasicAppletContainer.qml
    qml/ConfigOverlayWithHandles.qml
    qml/PlaceHolder.qml
)

ecm_target_qml_sources(containmentlayoutmanagerplugin
    PRIVATE
    SOURCES
    qml/private/BasicResizeHandle.qml
)

target_link_libraries(containmentlayoutmanagerplugin
        PUBLIC
            Qt::Core
        PRIVATE
            Qt::Qml Qt::Quick
            Plasma::Plasma Plasma::PlasmaQuick
    )

ecm_qt_declare_logging_category(containmentlayoutmanagerplugin
    HEADER containmentlayoutmanager_debug.h
    IDENTIFIER CONTAINMENTLAYOUTMANAGER_DEBUG
    CATEGORY_NAME org.kde.plasma.containmentlayoutmanager
    DESCRIPTION "plasma container layout manager"
    EXPORT PLASMAWORKSPACE
)

ecm_finalize_qml_module(containmentlayoutmanagerplugin)
