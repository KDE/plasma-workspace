# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>

add_executable(klipper-owner main.cpp databasejob.cpp)

target_link_libraries(klipper-owner Qt::Gui Qt::Sql KF6::GuiAddons KF6::CoreAddons KF6::KIOCore)

target_include_directories(klipper-owner PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/..")

install(TARGETS klipper-owner DESTINATION ${KDE_INSTALL_LIBEXECDIR})

ecm_qt_declare_logging_category(klipper-owner HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper DESCRIPTION "klipper" EXPORT KLIPPER)

