# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

# applets/clipboard
ecm_add_test(tst_clipboard.cpp TEST_NAME test_applet_clipboard
    LINK_LIBRARIES Qt::Qml Qt::Test Qt::Widgets KF5::I18n PW::PlasmaWindowed)
