# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
kcoreaddons_add_plugin(helprunner SOURCES helprunner.cpp helprunner.h INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(helprunner Qt::Widgets KF6::I18n KF6::Runner KF6::KIOGui)
