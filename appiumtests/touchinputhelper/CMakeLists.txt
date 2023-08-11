# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Threads REQUIRED)

set(touchinputhelperlib_SRCS
    touchinputhelper.cpp
)

add_library(touchinputhelper SHARED ${touchinputhelperlib_SRCS})
set_property(TARGET touchinputhelper PROPERTY POSITION_INDEPENDENT_CODE ON) # Required for ctypes
generate_export_header(touchinputhelper)

qt6_generate_wayland_protocol_client_sources(touchinputhelper FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml)

target_link_libraries(touchinputhelper
    PRIVATE
        Threads::Threads
        Qt::Core
        Qt::Gui # QGuiApplication
        Qt::GuiPrivate
        Qt::WaylandClient # Fake input protocol
        Wayland::Client
        PW::LibTaskManager # TasksModel
)

# Do not really install the library
if(DEFINED ENV{KDECI_BUILD})
install(TARGETS touchinputhelper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
else()
install(TARGETS touchinputhelper DESTINATION ${CMAKE_BINARY_DIR})
endif()