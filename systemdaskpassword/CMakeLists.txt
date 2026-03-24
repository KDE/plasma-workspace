# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

set(systemdaskpassword_SRCS askpasswordagent.cpp module.cpp)

ecm_qt_declare_logging_category(systemdaskpassword_SRCS
    IDENTIFIER SystemdAskPassword
    CATEGORY_NAME org.kde.systemdaskpassword
    HEADER systemdaskpassword_debug.h
    DESCRIPTION "Systemd password agent"
)

kcoreaddons_add_plugin(systemdaskpassword SOURCES ${systemdaskpassword_SRCS} INSTALL_NAMESPACE "kf6/kded")
target_compile_definitions(systemdaskpassword PRIVATE TRANSLATION_DOMAIN="plasma_systemdaskpassword")
target_link_libraries(systemdaskpassword
    Qt6::Widgets
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::WidgetsAddons
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
