# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause

ecm_add_test(calculatorrunnertest.cpp TEST_NAME calculatorrunnertest LINK_LIBRARIES Qt::Test Qt::Gui KF6::Runner KF6::KIOCore PkgConfig::QALCULATE)
krunner_configure_test(calculatorrunnertest calculator)
