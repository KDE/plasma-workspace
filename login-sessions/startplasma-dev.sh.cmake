#!/bin/bash

source @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh

# Copy systemd unit files in a custom prefix dir into the user's runtime directory

mkdir -p "$XDG_RUNTIME_DIR/systemd/user.control"
command cp -r @KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR@/* $XDG_RUNTIME_DIR/systemd/user.control
systemctl --user daemon-reload

startplasma$@
