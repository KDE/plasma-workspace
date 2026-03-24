<!--
    SPDX-License-Identifier: CC0-1.0
    SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>
-->

# Password Requests

The Password Requests KDED module displays per-user systemd password prompts
in Plasma sessions on Linux. It loads automatically at login and handles
requests in `$XDG_RUNTIME_DIR/systemd/ask-password/`. It requires access to
`pidfd_open()` to monitor requesters. Requests cannot be displayed if the
syscall is unavailable or blocked; the module logs a warning.

Use systemd 257 or later to request a password:

```sh
systemd-ask-password --user --no-tty
```

The module displays one prompt at a time. Other requests wait until the prompt
closes; requests that expire while waiting are skipped. A prompt closes when
its requester exits, its request file disappears, or its deadline expires.
Passwords may contain at most 2048 UTF-8 bytes. Request files may contain at
most 1 MiB.

If you publish request files directly, write each file completely before
renaming it into the request directory with a fresh randomized `ask.*` name.
Leave its contents unchanged until you remove it. Publish a new request file
to change the question or options.
