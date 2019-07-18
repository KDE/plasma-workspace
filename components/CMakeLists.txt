add_definitions(-DTRANSLATION_DOMAIN=\"plasmashellprivateplugin\")

install(DIRECTORY workspace/ DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/workspace/components)
add_subdirectory(containmentlayoutmanager)
add_subdirectory(shellprivate)
add_subdirectory(keyboardlayout)
add_subdirectory(sessionsprivate)
