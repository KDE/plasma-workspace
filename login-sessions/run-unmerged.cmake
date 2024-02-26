#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

set -ex

unshare -m -- @CMAKE_INSTALL_FULL_LIBEXECDIR@/run-unmerged-script "$@"
