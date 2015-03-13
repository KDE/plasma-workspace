set(batteryplugin_SRCS
    processrunner.cpp
    processrunnerplugin.cpp
    )

add_library(batteryplugin SHARED ${batteryplugin_SRCS})
target_link_libraries(batteryplugin Qt5::Core
                                    Qt5::Qml
                                    KF5::CoreAddons)

install(TARGETS batteryplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/battery)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/battery)
