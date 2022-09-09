add_library(kworkspaceplugin SHARED kworkspaceplugin.cpp)
target_link_libraries(kworkspaceplugin Qt::Qml kworkspace)

install(TARGETS kworkspaceplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kworkspace)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/kworkspace)
