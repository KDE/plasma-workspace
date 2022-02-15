

set(containmentlayoutmanagerplugin_SRCS
    containmentlayoutmanagerplugin.cpp
    appletcontainer.cpp
    configoverlay.cpp
    appletslayout.cpp
    abstractlayoutmanager.cpp
    gridlayoutmanager.cpp
    itemcontainer.cpp
    resizehandle.cpp
    )

add_library(containmentlayoutmanagerplugin ${containmentlayoutmanagerplugin_SRCS})

target_link_libraries(containmentlayoutmanagerplugin
        PUBLIC
            Qt::Core
        PRIVATE
            Qt::Qml Qt::Quick
            KF5::Plasma KF5::PlasmaQuick
    )

ecm_qt_declare_logging_category(containmentlayoutmanagerplugin
    HEADER containmentlayoutmanager_debug.h
    IDENTIFIER CONTAINMENTLAYOUTMANAGER_DEBUG
    CATEGORY_NAME org.kde.plasma.containmentlayoutmanager
)

install(TARGETS containmentlayoutmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/containmentlayoutmanager)

install(DIRECTORY qml/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/containmentlayoutmanager)
