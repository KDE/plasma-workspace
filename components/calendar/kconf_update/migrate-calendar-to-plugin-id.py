#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import re
import io
import subprocess

# removes the full path from calendar plugin entries and only stores the plugin id, e.g.
# /usr/lib64/qt5/plugins/plasmacalendarplugins/holidaysevents.so -> holidaysevents

proc = subprocess.Popen(
    [
        "qtpaths",
        "--locate-file",
        "ConfigLocation",
        "plasma-org.kde.plasma.desktop-appletsrc",
    ],
    stdout=subprocess.PIPE,
)
for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
    appletsrcPath = line.removesuffix("\n")

if len(appletsrcPath) == 0 or not appletsrcPath.endswith("appletsrc"):
    # something is wrong
    exit()

with open(appletsrcPath, "r+") as appletsrc:
    inputLines = appletsrc.readlines()

    outputLines = []
    pattern = re.compile("^\\/.*\\/(.*).so$")

    for line in inputLines:
        if not line.startswith("enabledCalendarPlugins="):
            outputLines += line
            continue

        inputPlugins = line.removeprefix("enabledCalendarPlugins=").split(",")

        outputPlugins = []

        for plugin in inputPlugins:
            match = pattern.match(plugin)

            if match:
                outputPlugins.append(match.group(1))
            else:
                outputPlugins += plugin

        outputLines += "enabledCalendarPlugins=" + ",".join(outputPlugins) + "\n"

    appletsrc.truncate(0)
    appletsrc.seek(0)

    appletsrc.writelines(outputLines)
