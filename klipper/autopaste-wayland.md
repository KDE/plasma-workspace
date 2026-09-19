<!--
    SPDX-License-Identifier: CC0-1.0
-->

# Automatic paste after selecting a Klipper entry

This implements the opt-in behavior requested by [Bug 427214](https://bugs.kde.org/show_bug.cgi?id=427214).

## Behavior

The setting is disabled by default. When enabled, selecting an item from the
clipboard history popup moves that item to the top of the history, waits until
the popup has lost focus, and sends the configured standard Paste shortcut to
the window that regained focus.

The history model emits a menu-only activation signal. This keeps automatic
paste out of other history operations such as editing an item, restoring
history, or receiving normal clipboard changes.

## Platform support

### Plasma Wayland

Klipper uses KDE's `org_kde_kwin_fake_input` protocol through KWayland. The
Plasma Shell desktop entry explicitly requests this privileged interface, and
Klipper authenticates it with the user-visible reason that it is performing an
opt-in automatic paste. No `xdotool`, `ydotool`, compositor-independent fake
input, or unrestricted portal session is used.

The protocol is KWin-specific. On another Wayland compositor, or on a Plasma
session where KWin does not expose the allowlisted interface, the option is not
shown as available and selecting a history item keeps the normal clipboard-only
behavior.

### X11

When the XTest extension is available, Klipper sends the user's first standard
Paste key binding through XTest. The option remains unavailable when XTest is
not present.

## Deliberate limitations

The operation sends a keyboard shortcut, not arbitrary MIME data directly into
an application. This means the target application decides how Paste behaves.
Terminal applications may require a different shortcut, and applications that
do not expose a focused paste target may ignore it. The feature is therefore
explicitly opt-in and disabled by default.

The implementation is intended for clipboard data that the target application's
normal Paste action supports. It does not bypass password field policy,
application permissions, or Wayland compositor focus rules.

## Relevant protocol history

KWin MR [!8236](https://invent.kde.org/plasma/kwin/-/merge_requests/8236)
explored a compositor-side `pasteIntoActiveWindow()` D-Bus method using text
input protocols. It was closed without merging and only covered UTF-8 text.
The implementation here uses the existing authenticated KWin fake-input path,
which preserves the application's normal Paste handling for text, images, files,
and custom MIME types where the application supports them.

## Manual checks

1. Leave the setting disabled and confirm selection only changes the clipboard.
2. Enable it on Plasma Wayland, copy two text entries, open Klipper, and select
   the older entry. Confirm the selected entry is pasted into the previous
   target after the popup closes.
3. Repeat in a Qt application, a GTK application, a terminal, a password field,
   and a permission dialog. Confirm unsupported targets remain controlled by
   their normal shortcut behavior.
4. Repeat with an image and a file clipboard entry where the target supports
   those paste formats.
5. Repeat on X11 with and without the XTest extension.
6. Confirm that keyboard, mouse, and accessibility selection paths all use the
   same opt-in behavior.
