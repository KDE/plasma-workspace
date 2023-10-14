#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from datetime import date
from dateutil.relativedelta import relativedelta
import time


class DigitalClockTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        desired_caps = {}
        desired_caps["app"] = "plasmawindowed -p org.kde.plasma.nano org.kde.plasma.digitalclock"
        desired_caps["timeouts"] = {'implicit': 10000}
        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            desired_capabilities=desired_caps)
        self.driver.implicitly_wait = 10
        # Open Applet
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="expandApplet").click()

    def setUp(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Today").click()
        self.assertEqual(self.compareMonthLabel(date.today()), True)

    def tearDown(self):
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    @classmethod
    def tearDownClass(self):
        self.driver.quit()

    def assertResult(self, actual, expected):
        wait = WebDriverWait(self.driver, 20)
        wait.until(lambda x: self.getresults() == expected)
        self.assertEqual(self.getresults(), expected)

    def compareMonthLabel(self, dateToTest):
        monthLabel = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="monthHeader")
        today = date.today()
        monthString = ""
        if dateToTest.year == today.year:
            monthString = dateToTest.strftime("%B")
        else:
            monthString = dateToTest.strftime("%B %Y")
        return monthLabel.text == monthString

    def test_next_month(self):
        nextMonthDate = date.today() + relativedelta(months=1)

        self.driver.find_element(by=AppiumBy.NAME, value="Next Month").click()

        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(nextMonthDate))
        self.assertEqual(self.compareMonthLabel(nextMonthDate), True)

    def test_prev_month(self):
        lastMonthDate = date.today() - relativedelta(months=1)

        self.driver.find_element(by=AppiumBy.NAME, value="Previous Month").click()

        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(lastMonthDate))
        self.assertEqual(self.compareMonthLabel(lastMonthDate), True)

    def test_months_view(self):
        dateAugust = date.today()
        dateAugust = dateAugust.replace(month = 8);

        self.driver.find_element(by=AppiumBy.NAME, value="Months").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}-{}".format(dateAugust.year, dateAugust.month)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateAugust))
        self.assertEqual(self.compareMonthLabel(dateAugust), True)

    def test_years_view(self):
        dateFuture = date.today() + relativedelta(years = 2)

        self.driver.find_element(by=AppiumBy.NAME, value="Years").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}".format(dateFuture.year)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateFuture))
        self.assertEqual(self.compareMonthLabel(dateFuture), True)

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(DigitalClockTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
