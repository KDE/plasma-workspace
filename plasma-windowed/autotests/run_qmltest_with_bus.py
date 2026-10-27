#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT
"""Run qmltestrunner against private D-Bus buses and optional mock services.

Most plasmoidtests only need the applet itself, but some applets show something
only when a D-Bus service is present: the media controller needs an MPRIS
player, the device notifier asserts the "no removable devices" state that only
holds when udisks2 is unreachable. qmltestrunner has no hook to set those up,
so such tests are wrapped by this script: it starts throwaway dbus-daemons,
runs the mock services on them, waits until the services own the bus names you
list, then executes the given command with DBUS_SESSION_BUS_ADDRESS /
DBUS_SYSTEM_BUS_ADDRESS pointing at the private buses.

Example (see CMakeLists.txt):

    run_qmltest_with_bus.py --session-bus \\
        --service "python3 .../mediaplayer.py .../player_a.json" \\
        --wait-name-prefix org.mpris.MediaPlayer2.appiumtest \\
        -- qmltestrunner -input mediacontroller_test.qml

The host's own buses are never touched: the private buses die with this
process, so a test always sees the same services regardless of what is running
or plugged into the machine.

Needs the dbus-daemon and dbus-send utilities (dbus-tools on some distros).
Service commands are parsed with shlex, so quote paths that contain spaces.
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from typing import Final

BUS_CONFIG: Final = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>{bus_type}</type>
  <listen>unix:tmpdir={directory}</listen>
  <policy context="default">
    <allow user="*"/>
    <allow own="*"/>
    <allow send_type="method_call"/>
    <allow send_type="signal"/>
    <allow send_type="error"/>
    <allow receive_type="method_call"/>
    <allow receive_type="signal"/>
    <allow receive_type="error"/>
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
  </policy>
</busconfig>
"""

_OWNED_NAME: Final = re.compile(r'string "([^"]+)"')


class PrivateBus:
    """A dbus-daemon listening on a throwaway unix socket."""

    def __init__(self, *, system: bool, directory: str) -> None:
        self._system = system
        self._directory = directory
        self._process: subprocess.Popen[bytes] | None = None
        self.address: str = ""

    @property
    def bus_type(self) -> str:
        return "system" if self._system else "session"

    def start(self) -> PrivateBus:
        config_path = os.path.join(self._directory, f"{self.bus_type}.conf")
        with open(config_path, "w", encoding="utf-8") as config_file:
            config_file.write(BUS_CONFIG.format(bus_type=self.bus_type, directory=self._directory))

        self._process = subprocess.Popen(
            ["dbus-daemon", f"--config-file={config_path}", "--nofork", "--print-address=1"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert self._process.stdout is not None
        line = self._process.stdout.readline()
        if not line:
            assert self._process.stderr is not None
            stderr = self._process.stderr.read().decode(errors="replace").strip()
            raise RuntimeError(f"could not start private {self.bus_type} bus: {stderr}")
        self.address = line.decode().strip()
        return self

    def stop(self) -> None:
        if self._process is None:
            return
        _terminate(self._process)
        self._process = None


def _terminate(process: subprocess.Popen) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def _owned_names(address: str) -> set[str]:
    """Names currently owned on the bus at address (empty on any failure)."""
    result = subprocess.run(
        [
            "dbus-send",
            f"--bus={address}",
            "--print-reply",
            "--dest=org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus.ListNames",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        timeout=5,
        check=False,
    )
    return set(_OWNED_NAME.findall(result.stdout))


def _wait_for_names(
    address: str,
    names: list[str],
    prefixes: list[str],
    timeout: float,
    services: list[subprocess.Popen],
) -> None:
    """Block until the bus owns every name and at least one name per prefix."""
    deadline = time.monotonic() + timeout
    while True:
        owned = _owned_names(address)
        missing = [name for name in names if name not in owned]
        missing += [prefix for prefix in prefixes if not any(name.startswith(prefix) for name in owned)]
        if not missing:
            return

        for service in services:
            if service.poll() is not None:
                raise RuntimeError(
                    f"mock service exited with code {service.returncode} before owning {', '.join(missing)}"
                )
        if time.monotonic() >= deadline:
            raise RuntimeError(f"timed out after {timeout:g}s waiting for {', '.join(missing)}")
        time.sleep(0.1)


def _start_bus(buses: dict[str, PrivateBus], *, system: bool, directory: str) -> None:
    # Register before starting: start() can be interrupted by SIGTERM, and the
    # cleanup in main() must still be able to stop the daemon.
    bus = PrivateBus(system=system, directory=directory)
    buses[bus.bus_type] = bus
    bus.start()


def _parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a command (usually qmltestrunner) on private D-Bus buses.",
        epilog="Pass the command to run after --, e.g. -- qmltestrunner -input test.qml",
    )
    parser.add_argument("--session-bus", action="store_true", help="start a private session bus")
    parser.add_argument("--system-bus", action="store_true", help="start a private system bus")
    parser.add_argument(
        "--service",
        action="append",
        default=[],
        metavar="CMD",
        help="mock service command to run on the private bus (repeatable)",
    )
    parser.add_argument(
        "--service-bus",
        choices=("session", "system"),
        default="session",
        help="bus the mock services run on (default: session)",
    )
    parser.add_argument(
        "--wait-name",
        action="append",
        default=[],
        metavar="NAME",
        help="bus name a mock must own before the command starts (repeatable)",
    )
    parser.add_argument(
        "--wait-name-prefix",
        action="append",
        default=[],
        metavar="PREFIX",
        help="like --wait-name, but any owned name with this prefix matches (repeatable)",
    )
    parser.add_argument("--wait-timeout", type=float, default=15, metavar="SECONDS")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="command to run, after --")

    args = parser.parse_args(argv)
    if args.command and args.command[0] == "--":
        args.command = args.command[1:]
    if not args.command:
        parser.error("no command given; pass it after --")

    started_buses = {"session": args.session_bus, "system": args.system_bus}
    if not started_buses[args.service_bus]:
        if args.service:
            parser.error(f"--service needs --{args.service_bus}-bus")
        if args.wait_name or args.wait_name_prefix:
            parser.error(f"--wait-name needs --{args.service_bus}-bus")
    return args


def main(argv: list[str] | None = None) -> int:
    args = _parse_args(sys.argv[1:] if argv is None else argv)

    # Turn SIGTERM into a normal exit so the cleanup below still runs when
    # CTest times the test out.
    signal.signal(signal.SIGTERM, lambda signum, _frame: sys.exit(128 + signum))

    directory = tempfile.mkdtemp(prefix="plasmoidtest-bus-")
    buses: dict[str, PrivateBus] = {}
    services: list[subprocess.Popen] = []
    try:
        if args.session_bus:
            _start_bus(buses, system=False, directory=directory)
        if args.system_bus:
            _start_bus(buses, system=True, directory=directory)

        env = os.environ.copy()
        if "session" in buses:
            env["DBUS_SESSION_BUS_ADDRESS"] = buses["session"].address
        if "system" in buses:
            env["DBUS_SYSTEM_BUS_ADDRESS"] = buses["system"].address

        for command in args.service:
            services.append(subprocess.Popen(shlex.split(command), env=env))

        if args.wait_name or args.wait_name_prefix:
            _wait_for_names(
                buses[args.service_bus].address,
                args.wait_name,
                args.wait_name_prefix,
                args.wait_timeout,
                services,
            )

        return subprocess.run(args.command, env=env).returncode
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    finally:
        for service in services:
            _terminate(service)
        for bus in buses.values():
            bus.stop()
        shutil.rmtree(directory, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
