# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

add_executable(askpasswordagenttest
    askpasswordagenttest.cpp
    ../askpasswordagent.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../systemdaskpassword_debug.cpp
)
target_include_directories(askpasswordagenttest PRIVATE .. ${CMAKE_CURRENT_BINARY_DIR}/..)
target_compile_definitions(askpasswordagenttest PRIVATE TRANSLATION_DOMAIN="plasma_systemdaskpassword")
target_link_libraries(askpasswordagenttest Qt6::Test Qt6::Widgets KF6::CoreAddons KF6::WidgetsAddons KF6::I18n)
add_test(NAME askpasswordagenttest
    COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen $<TARGET_FILE:askpasswordagenttest>)

find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
if(DBUS_RUN_SESSION_EXECUTABLE)
    add_executable(systemdaskpasswordmoduletest systemdaskpasswordmoduletest.cpp)
    target_link_libraries(systemdaskpasswordmoduletest Qt6::Test Qt6::Widgets KF6::CoreAddons KF6::DBusAddons KF6::I18n KF6::WidgetsAddons)
    add_dependencies(systemdaskpasswordmoduletest systemdaskpassword)
    add_test(NAME systemdaskpasswordmoduletest
        COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen
            PASSWORD_MODULE=$<TARGET_FILE:systemdaskpassword>
            ${DBUS_RUN_SESSION_EXECUTABLE} -- $<TARGET_FILE:systemdaskpasswordmoduletest>)
endif()
