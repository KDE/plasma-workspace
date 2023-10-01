#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from datetime import date
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from dateutil.relativedelta import relativedelta
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.digitalclock"


class DigitalClockTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
        # Open Applet
        cls.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="expandApplet").click()

    def setUp(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Today").click()
        self.assertEqual(self.compareMonthLabel(date.today()), True)

    def tearDown(self):
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_digitalclocktest_{self.id()}.png")

    @classmethod
    def tearDownClass(cls) -> None:
        """
        Make sure to terminate the driver again, lest it dangles.
        """
        cls.driver.quit()

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
        dateAugust = dateAugust.replace(month=8)

        self.driver.find_element(by=AppiumBy.NAME, value="Months").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}-{}".format(dateAugust.year, dateAugust.month)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateAugust))
        self.assertEqual(self.compareMonthLabel(dateAugust), True)

    def test_years_view(self):
        dateFuture = date.today() + relativedelta(years=2)

        self.driver.find_element(by=AppiumBy.NAME, value="Years").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}".format(dateFuture.year)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateFuture))
        self.assertEqual(self.compareMonthLabel(dateFuture), True)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(DigitalClockTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
