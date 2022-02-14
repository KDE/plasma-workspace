include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(jumplistplugin SHARED jumplistplugin.cpp)

target_link_libraries(jumplistplugin Qt::Qml jumplist)

install(TARGETS jumplistplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/jumplist)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/jumplist)
