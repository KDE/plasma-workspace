#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

import subprocess
import sys
import unittest
import time
import tempfile
import getpass
import argparse
import os

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

class LockscreenTests(unittest.TestCase):

    def createPamWrapperFiles(self, pammatrixpath):
        self.temp_dir = tempfile.TemporaryDirectory()
        # Get the current user's name
        user_name = getpass.getuser()

        # Write the "test_passwords" file
        with open(f"{self.temp_dir.name}/test_passwords", 'w') as passwords_file:
            passwords_file.write(f"{user_name}:mypassword:kde\n")

        for suffix in ["kde", "kde-fingerprint", "kde-smartcard"]:
            with open(f"{self.temp_dir.name}/{suffix}", 'w') as pam_file:
                pam_file.write(f'''
                    auth            required        {pammatrixpath} verbose
                    account         required        {pammatrixpath} verbose
                    password        required        {pammatrixpath} verbose
                    session         required        {pammatrixpath} verbose
            ''')

        print(f"Temporary directory created at: {self.temp_dir.name}")

    def setUp(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("--pammatrixpath", type=str, help="Path to PAM matrix")
        parser.add_argument("--kscreenlockergreetpath", type=str, help="Path to kscreenlocker greet script")
        args = parser.parse_args()

        self.createPamWrapperFiles(args.pammatrixpath);

        env = os.environ.copy()
        env["LD_PRELOAD"] = "libpam_wrapper.so"
        env["PAM_WRAPPER"] = "1"
        env["PAM_WRAPPER_SERVICE_DIR"] = self.temp_dir.name
        env["PAM_MATRIX_PASSWD"] = f"{self.temp_dir.name}/test_passwords"

        self.proc = subprocess.Popen([args.kscreenlockergreetpath, "--testing"], stdout=subprocess.PIPE, env=env)

        options = AppiumOptions()
        options.set_capability("app", str(self.proc.pid))
        options.set_capability("timeouts", {'implicit': 10000})
        self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)

    def tearDown(self):
        self.driver.quit()
        self.assertEqual(self.proc.returncode != None, True)
        try:
            self.proc.terminate(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file(f"failed_test_shot_lockscreen_{self.id()}.png")

    def assertStdoutLine(self, expected):
        out, err = self.proc.communicate()
        outLines = out.splitlines()
        self.assertEqual(expected, outLines[(len(outLines) - 1)].decode('UTF-8'))

    def test_login(self):
        time.sleep(1)
        self.driver.press_keycode("mypassword")
        time.sleep(1)
        self.assertStdoutLine("Unlocked")


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(LockscreenTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
