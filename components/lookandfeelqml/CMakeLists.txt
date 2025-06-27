ecm_add_qml_module(lookandfeelqmlplugin
    URI org.kde.plasma.lookandfeel
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

target_sources(lookandfeelqmlplugin PRIVATE
    kpackageinterface.cpp kpackageinterface.h
)

target_link_libraries(lookandfeelqmlplugin PRIVATE
    Qt::Qml
    KF6::ConfigCore
    KF6::Package
)

ecm_finalize_qml_module(lookandfeelqmlplugin)
