# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
ecm_add_test(locationsrunnertest.cpp TEST_NAME locationsrunnertest LINK_LIBRARIES Qt::Test Qt::Gui KF6::Runner KF6::KIOCore KF6::Service)
krunner_configure_test(locationsrunnertest locations)
