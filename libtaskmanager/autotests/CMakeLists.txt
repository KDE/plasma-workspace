set(samplewidgetwindowExecutablePath ${CMAKE_BINARY_DIR}/bin/samplewidgetwindow)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/windows/samplewidgetwindow.h.in
    ${CMAKE_BINARY_DIR}/samplewidgetwindow.h
    @ONLY
)
set(samplewidgetwindow_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/data/windows/samplewidgetwindow.cpp)
add_executable(samplewidgetwindow ${samplewidgetwindow_SOURCES})
target_link_libraries(samplewidgetwindow Qt::Core Qt::Gui)

ecm_add_tests(
    tasktoolstest.cpp
    tasksmodeltest.cpp
    launchertasksmodeltest.cpp
    LINK_LIBRARIES taskmanager Qt::Test KF6::Service KF6::ConfigCore
)

# Require QX11Info to set window state
ecm_add_test(xwindowtasksmodeltest.cpp LINK_LIBRARIES taskmanager Qt::Test Qt::GuiPrivate XCB::XCB KF6::Activities KF6::Service KF6::IconThemes KF6::WindowSystem)

# Require kwin_wayland, and KWIN_WAYLAND_NO_PERMISSION_CHECKS doesn't work on FreeBSD
if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()
add_executable(waylandtasksmodeltest waylandtasksmodeltest.cpp)
target_link_libraries(waylandtasksmodeltest PRIVATE taskmanager Qt::Test Qt::GuiPrivate Qt::WaylandClient Wayland::Client KF6::Service KF6::IconThemes KF6::WindowSystem)
# Do not really install the test
if(DEFINED ENV{KDECI_BUILD})
install(TARGETS waylandtasksmodeltest ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
else()
install(TARGETS waylandtasksmodeltest DESTINATION ${CMAKE_BINARY_DIR})
endif()

add_test(
    NAME waylandtasksmodeltest
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/waylandtasksmodeltestwrapper.py ${CMAKE_BINARY_DIR}/waylandtasksmodeltest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(waylandtasksmodeltest PROPERTIES TIMEOUT 60)
