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
if(QT_MAJOR_VERSION STREQUAL "6")
    ecm_add_test(xwindowtasksmodeltest.cpp LINK_LIBRARIES taskmanager Qt::Test Qt::GuiPrivate XCB::XCB KF6::Activities KF6::Service KF6::IconThemes KF6::WindowSystem)
else()
    ecm_add_test(xwindowtasksmodeltest.cpp LINK_LIBRARIES taskmanager Qt::Test Qt::X11Extras XCB::XCB KF6::Activities KF6::IconThemes KF6::Service KF6::WindowSystem)
endif()
