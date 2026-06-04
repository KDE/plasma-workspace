# SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>
# SPDX-License-Identifier: MIT

set(plasma_setup_xwayland_SRCS main.cpp)

add_executable(plasma-setup-xwayland ${plasma_setup_xwayland_SRCS})
ecm_mark_nongui_executable(plasma-setup-xwayland)

target_include_directories(plasma-setup-xwayland PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(plasma-setup-xwayland
    Qt::Core
    KF6::ConfigCore
    KF6::CoreAddons
)

if(HAVE_X11)
    target_link_libraries(plasma-setup-xwayland
        XCB::XCB
        XCB::CURSOR
    )
endif()

install(TARGETS plasma-setup-xwayland DESTINATION ${KDE_INSTALL_LIBEXECDIR})
