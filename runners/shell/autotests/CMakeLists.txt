# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lonau@gmx.de>
include(ECMAddTests)

ecm_add_test(shellrunnertest.cpp TEST_NAME shellrunnertest  LINK_LIBRARIES Qt::Test KF5::Runner)
target_compile_definitions(shellrunnertest PUBLIC -DPLUGIN_BUILD_DIR="${CMAKE_BINARY_DIR}/bin/kf5/krunner/" -DRUNNER_NAME="shell")
add_dependencies(shellrunnertest krunner_shell) # Test depends on the plugin being build
