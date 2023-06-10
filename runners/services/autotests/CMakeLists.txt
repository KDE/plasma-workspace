# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2016-2020 Harald Sitter <sitter@kde.org>

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

ecm_add_test(servicerunnertest.cpp TEST_NAME servicerunnertest
    LINK_LIBRARIES Qt::Test KF6::Service KF6::Runner)
krunner_configure_test(servicerunnertest krunner_services)
