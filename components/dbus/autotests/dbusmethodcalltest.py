#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
from typing import Final

CMAKE_BINARY_DIR: Final = os.getenv("CMAKE_BINARY_DIR")
QMLTEST_PATH: Final = os.getenv("QMLTEST_PATH")

if __name__ == '__main__':
    assert QMLTEST_PATH is not None
    assert os.path.exists(QMLTEST_PATH)
    result: int = 1

    with subprocess.Popen([os.path.join(os.path.dirname(os.path.abspath(__file__)), "dbusservice.py")], stdout=sys.stdout, stderr=sys.stderr) as dbus_service:
        args = [
            QMLTEST_PATH,
            "-input",
            os.path.join(os.path.dirname(os.path.abspath(__file__)), "dbusmethodcalltest.qml"),
        ]
        if CMAKE_BINARY_DIR is not None:
            args += ["-import", CMAKE_BINARY_DIR]

        test_runner = subprocess.Popen(args, stdout=sys.stdout, stderr=sys.stderr)
        try:
            result = test_runner.wait(timeout=60)
        except subprocess.TimeoutExpired as e:
            print("Timeout", e)
        finally:
            test_runner.terminate()
            dbus_service.terminate()

    sys.exit(result)
