#!/bin/bash

source @CMAKE_INSTALL_FULL_LIBEXECDIR@/plasma-dev-prefix.sh

# This is a bit of a hack done because systemd starts in pam, and we only set our dev paths after all that is complete
# This copies everything into a transient runtime directory that systemd reads and reloads the units

runtime_dbus_services=()
if [ ! -z  "$XDG_RUNTIME_DIR" ]; then
    mkdir -p "$XDG_RUNTIME_DIR/systemd/user.control"
    command cp -r @KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR@/* $XDG_RUNTIME_DIR/systemd/user.control
    systemctl --user daemon-reload

    # The session bus starts before the development paths are set too. Its
    # runtime service directory takes precedence over installed services.
    runtime_dbus_service_dir="$XDG_RUNTIME_DIR/dbus-1/services"
    mkdir -p "$runtime_dbus_service_dir"
    for service_file in "@KDE_INSTALL_FULL_DBUSDIR@/services/"*.service; do
        [ -f "$service_file" ] || continue
        service_name=$(sed -n 's/^Name=//p' "$service_file")
        runtime_service="$runtime_dbus_service_dir/$service_name.service"
        rm -f "$runtime_service"
        command cp "$service_file" "$runtime_service" && runtime_dbus_services+=("$runtime_service")
    done
    dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ReloadConfig > /dev/null
fi

trap 'kill -TERM $PID; wait $PID' TERM
startplasma$@ &
PID=$!
wait $PID

if [ ! -z  "$XDG_RUNTIME_DIR" ]; then
    cd @KDE_INSTALL_FULL_SYSTEMDUSERUNITDIR@
    for i in *; do
        rm -r $XDG_RUNTIME_DIR/systemd/user.control/$i
    done
    systemctl --user daemon-reload

    rm -f "${runtime_dbus_services[@]}"
    dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ReloadConfig > /dev/null
fi
