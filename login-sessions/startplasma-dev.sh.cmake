#!/bin/bash

# On-demand merge the systemd-sysext until systemd grows a builtin facility for this
# https://github.com/systemd/systemd/issues/25031
pkexec $(dirname "$0")/systemd-sysext-merge
function at_exit {
  pkexec $(dirname "$0")/systemd-sysext-unmerge
}
trap at_exit EXIT

source @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh

startplasma$@
