#!/bin/sh
set -e
# No need for sudo, since user is logged as root already
install @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev.desktop /usr/share/xsessions/
install @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev.desktop /usr/share/wayland-sessions/
install @CMAKE_BINARY_DIR@/prefix.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh
install @CMAKE_CURRENT_BINARY_DIR@/startplasma-dev.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@
