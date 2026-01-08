# SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
# SPDX-License-Identifier: BSD-2-Clause

set(startup_sound_SRCS main.cpp)

ecm_qt_declare_logging_category(startup_sound_SRCS HEADER debug.h IDENTIFIER PLASMA_STARTUP_SOUND CATEGORY_NAME org.kde.startup.sound)

add_executable(plasma-startup-sound ${startup_sound_SRCS})

target_link_libraries(plasma-startup-sound
    Qt6::Core
    KF6::ConfigCore
    KF6::Notifications
    Canberra::Canberra
)

install(TARGETS plasma-startup-sound DESTINATION ${KDE_INSTALL_FULL_LIBEXECDIR})

ecm_install_configured_files(
    INPUT
        plasma-startupsound.service.in
    DESTINATION
        ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
)
