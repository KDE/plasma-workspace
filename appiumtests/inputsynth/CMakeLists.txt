# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Threads REQUIRED)
find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)

add_library(inputsynth SHARED inputsynth.cpp)
set_target_properties(inputsynth PROPERTIES
    PREFIX ""
    OUTPUT_NAME "inputsynth"
    POSITION_INDEPENDENT_CODE ON
)

qt6_generate_wayland_protocol_client_sources(inputsynth FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml)

target_link_libraries(inputsynth
    PRIVATE
        Threads::Threads
        Qt::Core
        Qt::Gui # QGuiApplication
        Qt::WaylandClient # Fake input protocol
        Wayland::Client
        PW::LibTaskManager # TasksModel
        Python::Python
)

add_custom_command(TARGET inputsynth
    POST_BUILD
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/setup.py install --user
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
