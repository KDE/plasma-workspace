# SPDX-FileCopyrightText: 2026 Ramil Nurmanov <ramil2004nur@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(batterycontroltest
    batterycontroltest.cpp
    ../batterycontrol.cpp ../batterycontrol.h
    ../batteriesnamesmonitor_p.cpp ../batteriesnamesmonitor_p.h
    ../upowerbattery_p.cpp ../upowerbattery_p.h
    ../upowertypes_p.h
    ${CMAKE_CURRENT_BINARY_DIR}/../batterycontrol_debug.cpp
)
target_include_directories(batterycontroltest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(batterycontroltest PRIVATE Qt::DBus Qt::Qml Qt::Test KF6::I18n PW::KWorkspace)

add_test(NAME batterycontroltest COMMAND dbus-run-session -- $<TARGET_FILE:batterycontroltest>)
