#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import unittest
from typing import Final

from appium.webdriver.common.appiumby import AppiumBy

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import KCMTest

KCM_ID: Final = "kcm_users"


class KCMUsersTest(KCMTest):
    """
    Tests for kcm_users
    """

    kcm_id = KCM_ID

    def test_0_open(self) -> None:
        """
        Tests the KCM can be opened
        """
        self.driver.find_element(AppiumBy.NAME, "Add New User…")
        self.driver.find_element(AppiumBy.NAME, "Name:")
        self.driver.find_element(AppiumBy.NAME, "Change Password")
        self.driver.find_element(AppiumBy.NAME, "Delete User…")

    def test_1_pictures_sheet_1_open(self) -> None:
        """
        Opens the avatar sheet
        """
        self.driver.find_element(AppiumBy.NAME, "Change avatar").click()
        self.driver.find_element(AppiumBy.NAME, "Choose File…")
        #  Doesn't work without account service
        # self.driver.find_element(AppiumBy.NAME, "Initials")
        # self.driver.find_element(AppiumBy.NAME, "Placeholder Icon")


if __name__ == '__main__':
    unittest.main()
