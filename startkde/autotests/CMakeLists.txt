# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

if(NOT BUILD_TESTING)
    return()
endif()

ecm_add_tests(
    plasma-setup-live-system-test.cpp
    plasma-setup-live-user-test.cpp
    LINK_LIBRARIES Qt6::Test
)
