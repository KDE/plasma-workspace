# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Marco Martin <notmart@gmail.com>

set(ksecretprompter_SRCS
    main.cpp
    ksecretprompter.cpp
)

ecm_qt_declare_logging_category(ksecretprompter_SRCS
    IDENTIFIER "KSecretPrompterDaemon"
    CATEGORY_NAME "ksecretprompter"
    HEADER ksecretprompter_debug.h
    DESCRIPTION "KDE secretService prompter"
)

add_executable(ksecretprompter)

qt_add_dbus_adaptor(ksecretprompter_SRCS
    org.kde.secretprompter.xml
    ksecretprompter.h KSecretPrompter
)

target_sources(ksecretprompter PRIVATE
    ${ksecretprompter_SRCS}
)

target_link_libraries(ksecretprompter
    KF6::CoreAddons
    KF6::WidgetsAddons
    KF6::Crash
    KF6::I18n
    KF6::DBusAddons
    KF6::WindowSystem
    Qt6::Widgets
    Qt6::DBus
)

install(TARGETS ksecretprompter DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(org.kde.secretprompter.service.in
                ${CMAKE_CURRENT_BINARY_DIR}/org.kde.secretprompter.service)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.secretprompter.service
        DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

install(FILES org.kde.secretprompter.desktop DESTINATION ${KDE_INSTALL_APPDIR})
