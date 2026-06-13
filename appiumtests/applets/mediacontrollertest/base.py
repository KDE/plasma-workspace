#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import subprocess
import sys
import threading
import time
from typing import Final, IO

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils.base_test import PlasmaAppletTest

WIDGET_ID: Final = "org.kde.plasma.mediacontroller"
CURRENT_DIR: Final = os.path.dirname(os.path.abspath(__file__))
MEDIA_PLAYER_PATH: Final = os.path.join(CURRENT_DIR, "mediaplayer.py")

MPRIS_READY_MESSAGE: Final = "MPRIS registered"


class MediaControllerTestBase(PlasmaAppletTest):
    """
    Base class for media controller widget tests
    """

    widget_id = WIDGET_ID

    @staticmethod
    def _wait_for_ready(stderr: IO[bytes], proc: subprocess.Popen, timeout: float = 10.0) -> None:
        """
        Waits for the MPRIS player to register on D-Bus by monitoring stderr for the ready message.
        """
        deadline = time.time() + timeout
        ready_event = threading.Event()

        def reader_thread() -> None:
            try:
                for line in stderr:
                    decoded = line.decode("utf-8", errors="replace")
                    # Forward to stderr for debugging
                    sys.stderr.write(decoded)
                    sys.stderr.flush()
                    if MPRIS_READY_MESSAGE in decoded:
                        ready_event.set()
                        break
            except (ValueError, OSError):
                # Pipe closed or process terminated
                pass

        thread = threading.Thread(target=reader_thread, daemon=True)
        thread.start()

        while time.time() < deadline:
            if ready_event.is_set():
                return
            if proc.poll() is not None:
                raise RuntimeError(f"Player process exited unexpectedly with code {proc.returncode}")
            time.sleep(0.1)

        raise TimeoutError(f"Timeout waiting for MPRIS player to register on D-Bus after {timeout}s")

    @staticmethod
    def start_player(json_name: str, start_index: int = 0) -> subprocess.Popen:
        json_path = os.path.join(CURRENT_DIR, json_name)
        proc = subprocess.Popen(
            [sys.executable, MEDIA_PLAYER_PATH, json_path, "--start-index", str(start_index)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        # Wait for the player to register on D-Bus before returning
        MediaControllerTestBase._wait_for_ready(proc.stderr, proc)
        return proc

    @staticmethod
    def start_invalid_player() -> subprocess.Popen:
        return subprocess.Popen(
            [sys.executable, MEDIA_PLAYER_PATH, "--invalid"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    @staticmethod
    def stop_player(player: subprocess.Popen) -> None:
        player.terminate()
        try:
            player.wait(timeout=10)
        except subprocess.TimeoutExpired:
            player.kill()
            player.wait(timeout=5)
        finally:
            # Close pipes to release resources
            for pipe in (player.stdout, player.stderr):
                if pipe:
                    try:
                        pipe.close()
                    except OSError:
                        pass