# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

ecm_add_tests(regionandlangexampletest.cpp
    LINK_LIBRARIES kcm_regionandlang_example_static
    Qt::Test
    KF6::I18n
)
