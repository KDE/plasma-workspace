# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2016-2020 Harald Sitter <sitter@kde.org>

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Threads REQUIRED)
include(ECMAddTests)

ecm_add_test(servicerunnertest.cpp TEST_NAME servicerunnertest
    LINK_LIBRARIES Qt5::Test krunner_services_static Threads::Threads)
set_target_properties(servicerunnertest PROPERTIES CXX_STANDARD 17) # std::optional
