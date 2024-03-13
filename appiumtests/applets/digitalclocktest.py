#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import subprocess
import unittest
from datetime import date
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from dateutil.relativedelta import relativedelta
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.support.ui import WebDriverWait

WIDGET_ID: Final = "org.kde.plasma.digitalclock"


class DigitalClockTests(unittest.TestCase):

    driver: webdriver.Remote

    @classmethod
    def setUpClass(cls):
        options = AppiumOptions()
        options.set_capability("app", f"plasmawindowed -p org.kde.plasma.nano {WIDGET_ID}")
        options.set_capability("timeouts", {'implicit': 10000})
        cls.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
        # Open Applet
        cls.driver.find_element(AppiumBy.ACCESSIBILITY_ID, "digital-clock-compactrepresentation").click()

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

    def test_1_next_month(self):
        nextMonthDate = date.today() + relativedelta(months=1)

        self.driver.find_element(by=AppiumBy.NAME, value="Next Month").click()

        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(nextMonthDate))
        self.assertEqual(self.compareMonthLabel(nextMonthDate), True)

    def test_1_prev_month(self):
        lastMonthDate = date.today() - relativedelta(months=1)

        self.driver.find_element(by=AppiumBy.NAME, value="Previous Month").click()

        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(lastMonthDate))
        self.assertEqual(self.compareMonthLabel(lastMonthDate), True)

    def test_1_months_view(self):
        dateAugust = date.today()
        dateAugust = dateAugust.replace(month=8)

        self.driver.find_element(by=AppiumBy.NAME, value="Months").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}-{}".format(dateAugust.year, dateAugust.month)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateAugust))
        self.assertEqual(self.compareMonthLabel(dateAugust), True)

    def test_1_years_view(self):
        dateFuture = date.today() + relativedelta(years=2)

        self.driver.find_element(by=AppiumBy.NAME, value="Years").click()

        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="calendarCell-{}".format(dateFuture.year)).click()
        wait = WebDriverWait(self.driver, 50)
        wait.until(lambda x: self.compareMonthLabel(dateFuture))
        self.assertEqual(self.compareMonthLabel(dateFuture), True)

    def test_2_config_dialog_1_appearance(self) -> None:
        """
        Opens the config dialog
        """
        subprocess.check_call(["plasmawindowed", "--config"])
        wait = WebDriverWait(self.driver, 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Date format:")))

    def test_2_config_dialog_2_calendar_plugin_list(self) -> None:
        """
        Checks the calendar plugin list
        """
        self.driver.find_element(AppiumBy.NAME, "Calendar").click()
        wait = WebDriverWait(self.driver, 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Available Plugins:")))
        # Enable the plugin (BUG 480668)
        plugin_checkbox: WebElement = wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Holidays")))
        plugin_checkbox.click()
        # Switch back to Appearance so there will only be one "Holidays" match
        self.driver.find_element(AppiumBy.NAME, "Appearance").click()
        wait.until_not(lambda _: plugin_checkbox.is_displayed())
        # Switch to the calendar plugin
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Holidays"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Search")))

    def test_2_config_dialog_3_timezones(self) -> None:
        """
        Checks the timezone list
        """
        self.driver.find_element(AppiumBy.NAME, "Time Zones").click()
        wait = WebDriverWait(self.driver, 10)
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Add Time Zones…"))).click()
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "Add More Timezones")))
        self.driver.find_element(AppiumBy.NAME, "Search").send_keys("utc+03:00")
        wait.until(EC.presence_of_element_located((AppiumBy.NAME, "UTC+03:00")))


if __name__ == '__main__':
    unittest.main()
