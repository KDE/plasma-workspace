## Code Map

### `applets/`

Various applets. Look in the subdirectories for more info.

### `appmenu/`

The KDE Daemon (KDED) module for the global menu. Provides the org.kde.kappmenu and com.canonical.AppMenu.Registrar DBus names.

### `components/calendar/`

Calendar plugin, provides the org.kde.plasma.workspace.calendar QML import.

### `components/lookandfeelqml/`

Provides access to the Look and Feel KPackage from QML, through the org.kde.plasma.lookandfeel import.

### `components/dialogs/`

A component to create native dialogs. Provides the org.kde.plasma.workspace.dialogs import.

### `components/keyboardlayout/`

QML plugin for interacting with keyboard layouts and the virtual keyboard DBus interfaces provided by KWin. Provides the org.kde.plasma.workspace.keyboardlayout import. Used by the keyboardlayout applet in plasma-desktop.

### `containmentactions/`

Standard mouse actions on containments, such as wheel, right-click, middle-click, etc.

### `dataengines/`

DataEngines are plugins loaded on-demand to serve as data providers for use in plasmoids with a consistent interface. Various engines are implemented here. See also [the API documentation for the DataEngine class](https://api.kde.org/frameworks/plasma-framework/html/classPlasma_1_1DataEngine.html).

### `freespacenotifier/`

KDED module to warn the user when they're running out of space in their home directory.

### `gmenu-dbusmenu-proxy/`

Enables the appmenu-gtk-module for Gtk 2 and 3, and provides org.kde.plasma.gmenu_dbusmenu_proxy on DBus. Installs the gmenudbusproxy executable, an autostart desktop file, and a systemd service file.

### `interactiveconsole/`

The Plasma interactive console. Installs the plasma-interactiveconsole executable.

### `kcms/`

KCMs are KDE Config Modules, which show up in System Settings.

### `kioslave/applications/`

Provides the `applications:` and `programs:` protocols (both protocols are synonymous) for KIO.

### `kioslave/desktop/`

Provides the `desktop:` protocol for KIO, which gives access to the user's Desktop folder.

Also contains the Directory Watcher KDED module, which watches the user's Desktop folder, Trash folder, and their XDG user-dirs.dirs config for changes.

### `klipper/`

The clipboard manager for Plasma. This is the backend which provides the klipper executable as well as the org.kde.plasma.clipboard dataengine. See also `applets/clipboard/` for the applet UI.

### `krunner/`

The KRunner UI for Plasma. Provides the krunner executable, systemd service, and org.kde.krunner DBus service. See also the KRunner Framework.

### `ksmserver/`

KSMServer is KDE's session management server. See the documentation in that directory for more info.

### `ksplash/`

Provides the inital splash/loading screen for Plasma. Loads its UI from the current look-and-feel package.

### `ktimezoned/`

KDED module which watches for changes to the local timezone or the timezone database, and sends signals notifying those changes on the org.kde.KTimeZoned DBus name.

### `libcolorcorrect/`

Provides the sunrise/sunset time calculation and geolocation (through the geolocation dataengine) for Night Color.

The `declarative/` subdirectory provides the org.kde.colorcorrect QML import.

The `kded/` subdirectory provides the KDED module to periodically perform geolocation and send location updates to KWin if Night Color is set to Automatic location.

See also `kcms/nightcolor/`, KWin's `src/plugins/nightcolor/`, and kdeplasma-addons' `applets/nightcolor/`.

### `libdbusmenuqt/`

KDE's fork of Unity's libdbusmenu-qt.

### `libkworkspace/`

Library to interact with the KDE Session Manager.

### `libnotificationmanager/`

Library for handling notifications in Plasma. Backend for the Notifications applet.

### `libtaskmanager/`

Library which provides models for handling tasks: open windows (AbstractWindowTasksModel), startup notifications (StartupTasksModel) when launching an app, and app launchers (LauncherTasksModel).

### `login-sessions/`

Session files for Plasma Wayland and X11, and corresponding dev sessions for running a locally-built Plasma.

### `logout-greeter/`

The fullscreen menu which shows a selection of leaving options (Sleep, Shutdown, Logout, etc). Loads UI from the current look-and-feel package. Provides the ksmserver-logout-greeter executable.

### `lookandfeel/`

Contains the look-and-feel files for the Breeze, Breeze Dark, and Breeze Twilight themes. This includes the UI for the splash screen, window switcher, OSDs (like brightness or volume), the logout greeter, the lock screen, and desktop switcher.

Also contains the Breeze SDDM theme.

### `menu/`

Installs the default categories for app launcher menus.

### `phonon/`

KDE platform backend for [Phonon](https://userbase.kde.org/Phonon), a multimedia framework.

### `plasma-windowed/`

Provides the plasmawindowed executable, which can run plasmoids as standalone windows.

### `plasmacalendarintegration/`

Calendar plugin for displaying public holidays.

### `runners/`

Runners are plugins for KRunner, the launcher system used throughout Plasma.

### `sddm-wayland-session/`

Config files to make SDDM use KWin Wayland for its login greeter. Disabled by default for now, can be enabled by passing `INSTALL_SDDM_WAYLAND_SESSION` to CMake.

### `shell/`

The actual Plasma shell. Provides the plasmashell executable.

### `solidautoeject/`

KDED module to eject optical drives when their eject button is pressed.

### `soliduiserver/`

KDED module to show an authentication dialog for accessing hardware when needed.

### `startkde/`

The Plasma startup process.

### `statusnotifierwatcher/`

KDED module to keep track of StatusNotifierItems. Used by the system tray to actually display them. Provides org.kde.StatusNotifierWatcher.

See also [the StatusNotifierItem spec](https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/) on FreeDesktop.

### `systemmonitor/`

The Plasma System Monitor.

Also contains a KDED module to launch KSysGuard (System Activity) on Ctrl+Esc.

### `themes/`

Installs themerc files for the CDE, Cleanlooks, GTK+, Motif, Plastique, and MS Windows 9x Qt styles so they can show up and be selected in Application Styles (if available).

### `wallpapers/`

The Image and Plain Color wallpaper plugins for Plasma. Also provides the plasma-apply-wallpaperimage executable.

### `xembed-sni-proxy/`

Enables the tray icons of apps using the legacy XEmbed protocol to show up in the Plasma system tray by proxying it as an image over SNI. The directory has more documentation about how this works.

See also `statusnotifierwatcher/`.