set(appmenuapplet_SRCS
    appmenumodel.cpp
    appmenuplugin.cpp
)

add_library(appmenuplugin SHARED ${appmenuapplet_SRCS})
target_link_libraries(appmenuplugin
                      Qt5::Core
                      Qt5::Widgets
                      Qt5::Quick
                      KF5::Plasma
                      KF5::WindowSystem
                      PW::LibTaskManager
                      dbusmenuqt)

install(TARGETS appmenuplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/appmenu)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/appmenu)
