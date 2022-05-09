#!/bin/sh
# SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
# SPDX-FileCopyrightText: 2021 Nate Graham <nate@kde.org>
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -e

if [ "$(id -u)" != "0" ]; then
  kdialog --yesnocancel 'This will mangle your system dbus files! You also need to run this after each install to update your dbus data files and after distribution upgrades. Do not forget!'
  install @CMAKE_BINARY_DIR@/prefix.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh
  install @CMAKE_CURRENT_BINARY_DIR@/startplasma-dev.sh @CMAKE_INSTALL_FULL_LIBEXECDIR@
  pkexec ./$0
fi

# Clean up legacy stuff. Previously this script would try to massage things into shape through configs
# but that doesn't really work. See below.
if [ -d /opt/kde-dbus-scripts/ ]; then
  rm -rf /opt/kde-dbus-scripts/
  rm -f /etc/dbus-1/session.d/00-plasma.conf
fi

if [ -x /usr/bin/systemd-sysext ]; then
  prefix=/var/lib/extensions/plasma-dev
  install -d $prefix/usr/lib/extension-release.d/
  cp /etc/os-release $prefix/usr/lib/extension-release.d/extension-release.plasma-dev

  # Make built-from-source sessions appear in login screen
  install -d $prefix/usr/share/xsessions/
  install @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev6.desktop $prefix/usr/share/xsessions/
  install -d $prefix/usr/share/wayland-sessions/
  install @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev6.desktop $prefix/usr/share/wayland-sessions/

  # Copy dbus and polkit to /usr. Both hardcode the system prefix.
  # - polkit exclusively looks in the system prefix and has no facilities to change that
  #   https://gitlab.freedesktop.org/polkit/polkit/-/blob/92b910ce2273daf6a76038f6bd764fa6958d4e8e/src/polkitbackend/polkitbackendinteractiveauthority.c#L302
  # - dbus exclusively looks in the system prefix for **system** services and offers no facilities to change that
  #   https://gitlab.freedesktop.org/dbus/dbus/-/blob/9722d621497b2e7324e696f4095f56e2a9307a7e/bus/activation-helper.c#L422
  ln -sf @CMAKE_INSTALL_PREFIX@/@POLICY_FILES_INSTALL_DIR@/share/polkit-1/ $prefix/usr/share/polkit-1
  ln -sf @KDE_INSTALL_FULL_DBUSDIR@/ $prefix/usr/share/dbus-1

  systemd-sysext refresh
else # legacy compat
  install -D @CMAKE_CURRENT_BINARY_DIR@/plasmax11-dev6.desktop /usr/share/xsessions/
  install -D @CMAKE_CURRENT_BINARY_DIR@/plasmawayland-dev6.desktop /usr/share/wayland-sessions/

  cp -rv @CMAKE_INSTALL_PREFIX@/@POLICY_FILES_INSTALL_DIR@/share/polkit-1/* /usr/share/polkit-1/
  cp -rv @KDE_INSTALL_FULL_DBUSDIR@/* /usr/share/dbus-1/
fi
