# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

if(NOT BUILD_TESTING)
    return()
endif()

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_add_tests(oom_notifier-decodetest.cpp LINK_LIBRARIES Qt::Test KF6::Service)
target_include_directories(oom_notifier-decodetest PRIVATE ${CMAKE_SOURCE_DIR}/components)
