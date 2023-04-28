#CMakeLists.txt
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>


########### Build localegen helper ############
set(localegen_Helper_SRCS
    localegenhelper.cpp
)
qt_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/localegenhelper.h
    org.kde.localegenhelper.xml
    OPTIONS -s -m
)
qt_add_dbus_adaptor(localegen_Helper_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.localegenhelper.xml
                    ${CMAKE_CURRENT_SOURCE_DIR}/localegenhelper.h LocaleGenHelper)
add_executable(plasma-localegen-helper ${localegen_Helper_SRCS})
target_link_libraries(plasma-localegen-helper
    Qt::DBus
    PolkitQt6-1::Core
    KF6::I18n
)
ecm_install_configured_files(INPUT org.kde.localegenhelper.service.in DESTINATION ${KDE_INSTALL_DBUSSYSTEMSERVICEDIR} @ONLY)
install(TARGETS plasma-localegen-helper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

option(USE_CMAKE_PREFIX "Use CMAKE_INSTALL_PREFIX instead of `/usr` for DBus and PolicyKit config files" ON)
set(POLICY_FILES_INSTALL_DIR "/usr")
if(USE_CMAKE_PREFIX)
    if(NOT ${CMAKE_INSTALL_PREFIX} STREQUAL POLICY_FILES_INSTALL_DIR)
        MESSAGE(WARNING "INSTALL_BROKEN_POLICY_FILES is enabled. The following files will be installed to ${CMAKE_INSTALL_PREFIX} instead of ${POLICY_FILES_INSTALL_DIR}.")
    endif()
    set(POLICY_FILES_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
endif()

install(FILES org.kde.localegenhelper.conf DESTINATION "${POLICY_FILES_INSTALL_DIR}/share/dbus-1/system.d")
install(FILES org.kde.localegenhelper.policy DESTINATION "${POLICY_FILES_INSTALL_DIR}/share/polkit-1/actions")
