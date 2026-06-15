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
    launchertasksmodeltest.cpp
    LINK_LIBRARIES taskmanager Qt::Test KF6::Service KF6::ConfigCore
)

add_executable(tasksmodeltest tasksmodeltest.cpp)
target_link_libraries(tasksmodeltest PRIVATE taskmanager Qt::Test KF6::Service KF6::ConfigCore)

add_test(NAME tasksmodeltest COMMAND dbus-run-session kwin_wayland --exit-with-session=$<TARGET_FILE:tasksmodeltest>)
set_tests_properties(tasksmodeltest PROPERTIES ENVIRONMENT KWIN_WAYLAND_NO_PERMISSION_CHECKS=1)
