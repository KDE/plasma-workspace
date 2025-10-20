# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: None
[Desktop Entry]
Exec=@CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dbus-run-session-if-needed @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-run-in-safe-mode @CMAKE_INSTALL_FULL_LIBEXECDIR@/startplasma-dev.sh -wayland
DesktopNames=KDE
Name=Plasma (Development, Safemode, Wayland ${CMAKE_INSTALL_FULL_BINDIR})
Comment=Plasma by KDE
X-KDE-PluginInfo-Version=${PROJECT_VERSION}
X-KDE-HideFromAutologin=true
