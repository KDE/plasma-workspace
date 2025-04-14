ecm_add_qml_module(systemdialogs
    URI "org.kde.plasma.workspace.dialogs"
    GENERATE_PLUGIN_SOURCE
    VERSION 1.0
    QML_FILES SystemDialog.qml
    )

ecm_finalize_qml_module(systemdialogs)
