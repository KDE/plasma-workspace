include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(colorcorrectplugin SHARED colorcorrectplugin.cpp)

target_link_libraries(colorcorrectplugin
                      Qt5::Qml
                      colorcorrect)

install(TARGETS colorcorrectplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/colorcorrect)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/colorcorrect)
