<!--
    SPDX-License-Identifier: CC0-1.0
-->

# Automatic paste after selecting a Klipper entry

This note records the investigation for [Bug 427214](https://bugs.kde.org/show_bug.cgi?id=427214).
It is intentionally not an implementation of synthetic `Ctrl+V` input.

## Current behavior

Selecting an entry from the Klipper history moves it to the top of the history,
sets the system clipboard, and closes the popup. The selected data can then be
pasted by the user with the normal paste action. The selection path is:

1. `ClipboardItemDelegate.qml` emits `itemSelected`.
2. `ClipboardMenu.qml` calls `HistoryModel::moveToTop()`.
3. `KlipperPopup.qml` hides the popup.

There is no target window or target widget information in this path. The popup
is a separate Wayland client surface, so its focus is not the focus that was
active before the popup opened.

## Protocol investigation

### Wayland and text input

The Wayland clipboard protocol transfers data between clipboard owners and
requesting clients. It does not provide a request for a clipboard manager to
send a paste command to the client that previously had focus.

The relevant protocol specifications are [text-input-v3](https://wayland.app/protocols/text-input-unstable-v3)
and [input-method-v2](https://wayland.app/protocols/input-method-unstable-v2).
They are useful evidence for the direction of a future compositor-side API,
but they are not a paste API for ordinary clients.

The `zwp_text_input_v2` and `zwp_text_input_v3` protocols are different: they
let the compositor act as an input method for the currently focused surface.
The focused application must have enabled text input, and the compositor sends
text through the input-method protocol. Their `commit_string` events are sent
by an input method to the currently connected text input; they do not let an
ordinary client discover or retain an arbitrary child text widget, and they do
not define an operation for inserting clipboard MIME data into that widget.

Klipper is a regular Wayland client. It is not the compositor, an input method,
or the owner of the target application's text-input object. Calling text-input
interfaces from Klipper would therefore require an unsupported private
protocol, and would only cover plain text. It would not cover images, files,
rich text, terminals, or non-text controls.

### KWin and portals

KWin exposes the currently active window internally, but the current public KWin
D-Bus interface does not expose a paste-into-active-window operation. KWin MR
[!8236](https://invent.kde.org/plasma/kwin/-/merge_requests/8236) proposed
`pasteIntoActiveWindow()` using KWin's text-input implementation. The official
MR record shows that it was closed without being merged. Its stated support was
limited to UTF-8 plain text and text-input v1/v2/v3; images, rich text, files,
and custom MIME types were explicitly out of scope. The proposal therefore
cannot be used as a current plasma-workspace dependency.

The [RemoteDesktop portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html) can inject keyboard input only as part of a
user-authorized remote-control session. That permission and API are broader
than Klipper's history selection, do not identify the original text widget, and
would still have the same timing, password, image, and non-text limitations.
It is not an appropriate implementation dependency for Klipper.

### X11 and synthetic keys

X11-specific fake-key approaches can work in some applications, but they are
not equivalent to a paste operation: `Ctrl+V`, `Shift+Insert`, terminal
shortcuts, password fields, and application-specific bindings do not have one
portable meaning. They can also target a different window if focus changes
while the popup is closing. This is precisely the approach rejected by the
maintainer discussion in Bug 427214, where simulated key presses were described
as a hack that would not be accepted in the codebase.

## Bug discussion status

The complete Bugzilla discussion shows a consistent distinction between the
useful requested behavior and the lack of a safe implementation:

* In October 2020, the initial request proposed triggering the system paste
  shortcut and the maintainer replied that it might work some of the time but
  was a hack that would not be accepted in the codebase.
* In July 2022, a CopyQ-style `sendKeyPress` approach was proposed and the same
  objection was reaffirmed.
* In September 2023, the report pointed to compositor input-method work as a
  possible direction, rather than suggesting that Klipper inject keys itself.
* Bug 474671, Bug 478392, Bug 478870, Bug 487637, and Bug 515755 are marked as
  duplicates of this report. Bug 514095 is a separate clipboard-history image
  issue and is not evidence for automatic paste.
* KWin MR !8236 was opened in October 2025, proposed a KWin D-Bus operation,
  and was closed without merge in December 2025. Its author later described
  the plain-text-only result as hacky as well.

The report remains confirmed and unresolved. The discussion establishes the
need, but does not establish an API that Klipper can safely call.

## Decision

Do not add a setting that claims to provide automatic paste while relying on
`xdotool`, `ydotool`, XTest, Qt key events, or an unmerged KWin D-Bus method.
Making that setting default-off would not make the operation reliable or make
it safe for password fields, permission dialogs, terminals, images, or files.

The existing behavior remains the safe fallback when the target cannot be
identified or when the selected entry is not plain text. No source or
configuration change is made for this feature until a supported compositor
API exists.

This note is the complete contribution from the Klipper side for the current
protocol landscape: it documents why the feature request is valid and why an
apparently small shortcut implementation would not be an upstream-quality fix.
It should be revisited when KWin offers a supported, user-authorized operation
with a defined target, MIME-data, and security contract.

## Maintainer-oriented follow-up

A future implementation should be designed across KWin and Plasma rather than
in Klipper alone. Before Klipper integration, that work needs to specify:

* how a user-initiated selection is authorized and bound to the original seat
  focus, without restoring focus to an unrelated window;
* how KWin identifies an eligible text input and rejects password, terminal,
  permission, and non-text controls when appropriate;
* how clipboard MIME data is transferred, including images and files, without
  reducing the feature to plain-text injection; and
* what happens when the original surface is gone, focus changed, the clipboard
  is still being served asynchronously, or the compositor is X11 rather than
  Wayland.

Only after that API is stable should Klipper record a target context before
opening the popup, wait for the selected clipboard offer to be available, and
request the compositor operation. Until then, external tools may implement
their own policy using Klipper's existing clipboard-history notifications, but
that policy must remain outside Klipper.

## Manual validation

The following checks verify that the current behavior is preserved while this
investigation is reviewed:

1. On a Wayland text field, copy two text entries, open Klipper with the
   history shortcut, and click the older entry. Confirm that the popup closes,
   `wl-paste` reports the selected entry, and the target field is unchanged
   until the user invokes paste.
2. Repeat with a password field, terminal, image entry, file entry, and a
   permission dialog. Confirm that no synthetic key event or unexpected text is
   sent to the target.
3. Repeat the text-field check under X11. Confirm that selecting an entry only
   changes the clipboard and preserves the existing explicit-paste workflow.
4. Select an entry by mouse, keyboard, and accessibility activation. Confirm
   that all paths have the same clipboard and popup behavior.

These checks should be rerun when a supported KWin operation is available; they
are not evidence that automatic paste is currently implemented.
