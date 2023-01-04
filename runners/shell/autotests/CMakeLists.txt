# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lonau@gmx.de>
include(ECMAddTests)

ecm_add_test(shellrunnertest.cpp TEST_NAME shellrunnertest  LINK_LIBRARIES Qt::Test Qt::Gui KF5::Runner)
configure_krunner_test(shellrunnertest krunner_shell)
