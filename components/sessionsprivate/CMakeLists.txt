set(sessionsprivateplugin_SRCS
    sessionsprivateplugin.cpp
)

add_library(sessionsprivateplugin SHARED ${sessionsprivateplugin_SRCS})
target_link_libraries(sessionsprivateplugin
    Qt::Qml
    PW::KWorkspace
)

install(TARGETS sessionsprivateplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/sessions)

