ecm_add_qml_module(workspacecomponents
    URI "org.kde.plasma.workspace.components"
    VERSION 2.0
    GENERATE_PLUGIN_SOURCE
)

ecm_target_qml_sources(workspacecomponents
    SOURCES
    BatteryIcon.qml
    KeyboardLayoutSwitcher.qml
    BadgeOverlay.qml
)

ecm_finalize_qml_module(workspacecomponents)
