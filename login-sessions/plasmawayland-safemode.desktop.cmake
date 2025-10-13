[Desktop Entry]
Exec=@CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dbus-run-session-if-needed @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-run-in-safe-mode ${CMAKE_INSTALL_FULL_BINDIR}/startplasma-wayland
TryExec=${CMAKE_INSTALL_FULL_BINDIR}/startplasma-wayland
DesktopNames=KDE
Name=Plasma (Wayland + Safe Mode)
Comment=Plasma by KDE in Safe Mode
X-KDE-PluginInfo-Version=${PROJECT_VERSION}
X-KDE-HideFromAutologin=true
