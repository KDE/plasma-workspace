#!/bin/sh
# SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
# SPDX-FileCopyrightText: 2021 Nate Graham <nate@kde.org>
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -e

if [ "$(id -u)" != "0" ]; then
  install @CMAKE_BINARY_DIR@/prefix.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh
  install @CMAKE_CURRENT_BINARY_DIR@/startplasma-dev.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@
  exec pkexec ./$0
fi

# Clean up legacy stuff. Previously this script would try to massage things into shape through configs
# but that doesn't really work. See below.
if [ -d /opt/kde-dbus-scripts/ ]; then
  rm -rf /opt/kde-dbus-scripts/
  rm -f /etc/dbus-1/session.d/00-plasma.conf
fi

if [ -x /usr/bin/systemd-sysext ]; then
  mkdir -p @CMAKE_INSTALL_PREFIX@/lib/extension-release.d/
  install --mode=644 -D /usr/lib/os-release @CMAKE_INSTALL_PREFIX@/lib/extension-release.d/extension-release.plasma-dev

  prefix=/var/lib/extensions/
  mkdir -p $prefix
  ln -sf @CMAKE_INSTALL_PREFIX@/../ $prefix/plasma-dev

  # Make built-from-source sessions appear in login screen
  install --mode=644 -D @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev.desktop --target-directory=$prefix/usr/share/xsessions
  install --mode=644 -D @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev.desktop --target-directory=$prefix/usr/share/wayland-sessions

  systemd-sysext refresh
  systemctl enable systemd-sysext.service
else # legacy compat
  echo 'Only systemd based systems are supported. You need systemd-sysext on your system'
fi

