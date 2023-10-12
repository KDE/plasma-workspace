#!/usr/bin/env bash
# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0

find . -type f -name "*.cursor" -exec sh -c 'xcursorgen {} "../$(basename {} .cursor)"' \;
