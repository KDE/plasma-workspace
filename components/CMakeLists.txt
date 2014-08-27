install(DIRECTORY qml/ DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/workspace/components)

add_subdirectory(interactiveconsole)
if (KF5TextEditor_FOUND)
add_subdirectory(widgetexplorer)
endif()
