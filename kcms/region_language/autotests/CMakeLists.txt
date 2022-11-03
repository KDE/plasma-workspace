#CMakeLists.txt
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>
include(ECMAddTests)

ecm_add_tests(regionandlangexampletest.cpp
    LINK_LIBRARIES kcm_regionandlang_example_static
    Qt::Test
    KF5::I18n
)
