# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT
"""Process management utilities for tests."""

import subprocess
import time

from .constants import KDE_VERSION, PROCESS_WAIT_INTERVAL, PROCESS_WAIT_ITERATIONS


def quit_plasmawindowed() -> None:
    """Quit plasmawindowed process."""
    subprocess.check_call([f"kquitapp{KDE_VERSION}", "plasmawindowed"])


def wait_for_process_exit(process_name: str) -> None:
    """Wait for a process to exit."""
    for _ in range(PROCESS_WAIT_ITERATIONS):
        try:
            subprocess.check_call(["pidof", process_name])
        except subprocess.CalledProcessError:
            break
        time.sleep(PROCESS_WAIT_INTERVAL)


def cleanup_plasmawindowed() -> None:
    """Clean up plasmawindowed process."""
    quit_plasmawindowed()
    wait_for_process_exit("plasmawindowed")
