#!/bin/bash

# On-demand merge the systemd-sysext until systemd grows a builtin facility for this
# https://github.com/systemd/systemd/issues/25031
pkexec $(dirname "$0")/systemd-sysext-merge
function at_exit {
  pkexec $(dirname "$0")/systemd-sysext-unmerge
}
trap at_exit EXIT

source @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh

# This is a bit of a hack done because systemd starts in pam, and we only set our dev paths after all that is complete
# This copies everything into a transient runtime directory that systemd reads and reloads the units

if [ ! -z  "$XDG_RUNTIME_DIR" ]; then
    mkdir -p "$XDG_RUNTIME_DIR/systemd/user.control"
    command cp -r @KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR@/* $XDG_RUNTIME_DIR/systemd/user.control
    systemctl --user daemon-reload
fi

startplasma$@

if [ ! -z  "$XDG_RUNTIME_DIR" ]; then
    cd @KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR@
    for i in *; do
        rm -r $XDG_RUNTIME_DIR/systemd/user.control/$i
    done
    systemctl --user daemon-reload
fi
