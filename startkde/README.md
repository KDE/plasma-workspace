## StartPlasma

`startplasma` is the main binary invoked by the login manager. It is compiled as two binaries (`startplasma-x11` and `startplasma-wayland`) with some common files. It sets up key environment variables and other core GUI-less tasks, and then triggers the main startup sequence either via requesting the plasma systemd target, or a legacy fallback via plasma-session.

`startplasma` tracks the status of ksmserver (x11) or kwin_wayland (wayland) to detect when to exit. To ensure all shutdown tasks are complete, it also waits until `plasma-shutdown` exits.

## plasma-dbus-run-session-if-needed

This is a workaround script for some setups. See 8475fe4545998c806704a45a7d912f777a11533f

## plasma-shutdown

`plasma-shutdown` is a small helper invoked via DBus activation after any potential logout confirmation by the user.
It triggers the relevant paths to quit and save all apps via both ksmserver and KWin, abstracting the session differences between X11 and Wayland. If this is successful, then any shutdown scripts are run, and finally KWin and ksmserver are terminated. After returning success or failure, the helper exits.

### plasma-session

`plasma-session` is the legacy fallback for non-systemd users. It launches KWin, Plasma, and others in the intended order.
It should behave as similarly to the systemd use case as possible.

## session-shortcuts

`session-shortcuts` is a small KDED module that registers global shortcuts for session actions like requesting logout or shutdown, triggering the relevant libkworkspace calls to invoke them.

## session-restore

Session restore contains two binaries: `plasma-fallback-session-restore` and `plasma-fallback-session-save`. This works by recording a list of running applications on save. It runs after ksmserver to avoid recording apps compatible with session restore, which were already recorded by ksmserver. Restoration is invoked via the typical autostart mechanism.

## kcminit

`kcminit` loads plugins to perform one-shot operations to set up a session. It is scheduled for future deprecation; we plan to replace existing kcminits with KDED modules. However this has not been completed yet.

## waitforname

`waitforname` is a DBus trick to ensure that any messages to system-invoked services are not lost. It marks itself as DBus-activatable for the relevant service names, but doesn't register those names itself. This makes the DBus daemon keep the messages queued until the real `plasmashell` starts and a notification server is running, at which point the messages are shown to the user via standard Plasma notifications.
It is currently used by `plasmashell`, the notifications API, and `ksplash` for the ready signals.

## plasma-sourceenv.sh

This is for dev sessions only, and sets up environment variables to make a custom installation directory work.

# Live Session setup

To help with distro live session setup we have tools to help carry out common actions
such as disabling a bunch of services or switching plasma-welcome into live mode.
Merge requests welcome!

All tools support the SYSROOT environment variable to switch to a different root. Implicit default is /.
Please beware that for the user tool HOME does override SYSROOT when we write or read assets from the home.

- plasma-setup-live-system: should be run as root and configures the system portion.
  Requires USERNAME to be set to the target username for sddm setup.
  Alternatively the kernel command line argument plasma.live.user=foobar may be set.
- plasma-setup-live-user: should be run as the live session user and configures the user portion.
  Requires HOME to be set implicitly.

There are also supporting systemd units. They only activate if plasma.live.user is set on the cmdline.
