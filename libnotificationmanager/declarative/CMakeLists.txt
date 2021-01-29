include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(notificationmanagerplugin SHARED notificationmanagerplugin.cpp)

target_link_libraries(notificationmanagerplugin Qt::Qml notificationmanager)

install(TARGETS notificationmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/notificationmanager)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/notificationmanager)

