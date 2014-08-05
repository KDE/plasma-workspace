set(batteryplugin_SRCS
    processrunner.cpp
    processrunnerplugin.cpp
    )

add_library(batteryplugin SHARED ${batteryplugin_SRCS})
target_link_libraries(batteryplugin Qt5::Core
                                    Qt5::Qml
                                    KF5::CoreAddons)

install(TARGETS batteryplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/battery)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/battery)
