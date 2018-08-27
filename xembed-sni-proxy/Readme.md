##XEmbed SNI Proxy

The goal of this project is to make xembed system trays available in Plasma.

This is to allow legacy apps (xchat, pidgin, tuxguitar) etc. system trays[1] available in Plasma which only supports StatusNotifierItem [2].

Ideally we also want this to work in an xwayland session, making X system tray icons available even when plasmashell only has a wayland connection.

This project should be portable onto all other DEs that speak SNI.

##How it works (in theory)

* We register a window as a system tray container
* We render embedded windows composited offscreen
* We render contents into an image and send this over DBus via the SNI protocol
* XDamage events trigger a repaint
* Activate and context menu events are replyed via X send event into the embedded container as left and right clicks

There are a few extra hacks in the real code to deal with some toolkits being awkward.

##Build instructions

    cmake .
    make
    sudo make install

After building, run `xembedsniproxy`.

[1] http://standards.freedesktop.org/systemtray-spec/systemtray-spec-latest.html
[2] http://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/
