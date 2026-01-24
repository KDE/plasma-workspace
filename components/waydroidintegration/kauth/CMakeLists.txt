# SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(waydroidhelper)
target_sources(waydroidhelper PRIVATE waydroidhelper.cpp)

target_link_libraries(waydroidhelper
    Qt6::Core
    KF6::AuthCore
    KF6::CoreAddons
)

install(TARGETS waydroidhelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})

find_package(KF6Auth NO_MODULE)

kauth_install_helper_files(waydroidhelper org.kde.plasma.workspace.waydroidhelper root)
kauth_install_actions(org.kde.plasma.workspace.waydroidhelper waydroidhelper.actions)

ecm_qt_declare_logging_category(waydroidhelper
    HEADER waydroidhelper_debug.h
    IDENTIFIER WAYDROIDHELPER
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME org.kde.plasma.workspace.waydroidhelper
    DESCRIPTION "Helper for Waydroid for some actions need root access"
)
