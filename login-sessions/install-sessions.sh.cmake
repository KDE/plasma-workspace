#!/bin/sh
set -e

sudo install @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev.desktop /usr/share/xsessions/
sudo install @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev.desktop /usr/share/wayland-sessions/
install @CMAKE_BINARY_DIR@/prefix.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh
install @CMAKE_CURRENT_BINARY_DIR@/startplasma-dev.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@
