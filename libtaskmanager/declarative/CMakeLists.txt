include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(taskmanagerplugin SHARED taskmanagerplugin.cpp)

target_link_libraries(taskmanagerplugin Qt5::Qml
                                          taskmanager)

install(TARGETS taskmanagerplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/taskmanager)

