kcoreaddons_add_plugin(plasma_containmentactions_paste SOURCES paste.cpp INSTALL_NAMESPACE "plasma/containmentactions")
target_link_libraries(plasma_containmentactions_paste
    Qt::Gui
    Qt::Widgets
    KF5::Plasma
    KF5::KIOCore
)

ecm_qt_declare_logging_category(plasma_containmentactions_paste
    HEADER containmentactions_paste_debug.h
    IDENTIFIER CONTAINMENTACTIONS_PASTE_DEBUG
    CATEGORY_NAME org.kde.plasma.containmentactions_paste
)
