#!/bin/sh
# SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
# SPDX-FileCopyrightText: 2021 Nate Graham <nate@kde.org>
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -ex

if [ "$(id -u)" != "0" ]; then
  install @CMAKE_BINARY_DIR@/prefix.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh
  install @CMAKE_CURRENT_BINARY_DIR@/startplasma-dev.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@
  install --mode=755 -D @CMAKE_CURRENT_SOURCE_DIR@/systemd-sysext-merge --target-directory=@CMAKE_INSTALL_FULL_LIBEXECDIR@
  install --mode=755 -D @CMAKE_CURRENT_SOURCE_DIR@/systemd-sysext-unmerge --target-directory=@CMAKE_INSTALL_FULL_LIBEXECDIR@
  install --mode=755 @CMAKE_CURRENT_BINARY_DIR@/run-unmerged @CMAKE_INSTALL_FULL_BINDIR@
  install --mode=755 @CMAKE_CURRENT_SOURCE_DIR@/run-unmerged-script @CMAKE_INSTALL_FULL_LIBEXECDIR@
  exec pkexec @CMAKE_INSTALL_FULL_BINDIR@/run-unmerged $(dirname "$0")/install-sessions.sh
fi

# Clean up legacy stuff. Previously this script would try to massage things into shape through configs
# but that doesn't really work. See below.
if [ -d /opt/kde-dbus-scripts/ ]; then
  rm -rf /opt/kde-dbus-scripts/
  rm -f /etc/dbus-1/session.d/00-plasma.conf
fi

# - polkit exclusively looks in the system prefix and has no facilities to change that
#   https://gitlab.freedesktop.org/polkit/polkit/-/blob/92b910ce2273daf6a76038f6bd764fa6958d4e8e/src/polkitbackend/polkitbackendinteractiveauthority.c#L302
# - dbus exclusively looks in the system prefix for **system** services and offers no facilities to change that
#   https://gitlab.freedesktop.org/dbus/dbus/-/blob/9722d621497b2e7324e696f4095f56e2a9307a7e/bus/activation-helper.c#
if [ -x /usr/bin/systemd-sysext ]; then
  install --mode=644 -D @CMAKE_CURRENT_SOURCE_DIR@/extension-release.plasma-dev6 --target-directory=@CMAKE_INSTALL_PREFIX@/lib/extension-release.d
  install --mode=644 -D @CMAKE_CURRENT_SOURCE_DIR@/extension-release.plasma-dev6 --target-directory=@CMAKE_INSTALL_PREFIX@/etc/extension-release.d

  prefix=/var/local/extensions-plasma-dev6
  rm -rf $prefix
  ln -sf @CMAKE_INSTALL_PREFIX@/../ $prefix

  # Let us pkexec our merge and unmerge
  install --mode=644 -D @CMAKE_CURRENT_BINARY_DIR@/org.kde.plasma.systemd-sysext.policy --target-directory=/usr/share/polkit-1/actions

  # Make built-from-source sessions appear in login screen
  install --mode=644 -D @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev6.desktop --target-directory=/usr/local/share/xsessions
  install --mode=644 -D @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev6.desktop --target-directory=/usr/local/share/wayland-sessions
else # legacy compat
  echo 'Only systemd based systems are supported. You need systemd-sysext on your system or figure things out for yourself.'
fi

