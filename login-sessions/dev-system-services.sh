#!/bin/bash
# SPDX-FileCopyrightText: 2023 Jakob Petsovits <jpetso@petsovits.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -e

SYSTEM_DBUS_BASEDIR=/usr/local/share/dbus-1
SYSTEM_DBUS_SERVICES=${SYSTEM_DBUS_BASEDIR}/system-services
SYSTEM_DBUS_CONFIGS=${SYSTEM_DBUS_BASEDIR}/system.d

KDE_DBUS_SERVICES=/opt/kde-dbus-scripts/system-services
KDE_DBUS_CONFIGS=/opt/kde-dbus-scripts/system.d

function usage()
{
    echo "Usage: $0 [enable|disable]"
    echo ""
    echo "    Install built system services to ${SYSTEM_DBUS_BASEDIR} or remove them again."
    echo "    Installing services will prioritize them over OS-provided services of the same name"
    echo "    and keep certain parts of a co-installed Plasma 5 desktop from working correctly."
}

if [[ $# -eq 1 && ( "$1" == "--help" || "$1" == "-h" ) ]]; then

    usage

elif [[ $# -eq 1 && "$1" == "enable" ]]; then

    if [[ ! -d "${KDE_DBUS_SERVICES}" || ! -d "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Error: ${KDE_DBUS_SERVICES} doesn't exist - aborting"
        echo "       Use install-sessions.sh at least once to set up the folder structure in /opt/kde-dbus-scripts"
        exit 1
    fi

    # symlink services
    if [[ -L "${SYSTEM_DBUS_SERVICES}" && $(readlink -f "${SYSTEM_DBUS_SERVICES}") == "${KDE_DBUS_SERVICES}" ]]; then
        echo "Skipping services - already symlinked"
    else
        echo "Symlinking services"
        if [[ ! -d "${SYSTEM_DBUS_SERVICES}" ]]; then
            sudo mkdir -p "${SYSTEM_DBUS_SERVICES}"
        fi
        for kde_service in "${KDE_DBUS_SERVICES}"/*; do
            service=$(basename "$kde_service")
            system_service="${SYSTEM_DBUS_SERVICES}"/"${service}"
            if [[ -e "${system_service}" ]]; then
                if [[ -L "${system_service}" && $(readlink -f "${system_service}") == "${kde_service}"  ]]; then
                    echo "Skipping ${service} - already symlinked"
                else
                    echo "Warning: Skpping ${service} - exists but is not a symlink"
                fi
            else
                sudo ln -s "${kde_service}" "${system_service}"
                echo "Symlinked ${service}"
            fi
        done
    fi

    # symlink configs
    if [[ -L "${SYSTEM_DBUS_CONFIGS}" && $(readlink -f "${SYSTEM_DBUS_CONFIGS}") == "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Skipping configs - already symlinked"
    else
        echo "Symlinking configs"
        if [[ ! -d "${SYSTEM_DBUS_CONFIGS}" ]]; then
            sudo mkdir -p "${SYSTEM_DBUS_CONFIGS}"
        fi
        for kde_config in "${KDE_DBUS_CONFIGS}"/*; do
            config=$(basename "$kde_config")
            system_config="${SYSTEM_DBUS_CONFIGS}"/"${config}"
            if [[ -e "${system_config}" ]]; then
                if [[ -L "${system_config}" && $(readlink -f "${system_config}") == "${kde_config}"  ]]; then
                    echo "Skipping ${config} - already symlinked"
                else
                    echo "Warning: Skpping ${config} - exists but is not a symlink"
                fi
            else
                sudo ln -s "${kde_config}" "${system_config}"
                echo "Symlinked ${config}"
            fi
        done
    fi

    echo "Done"

elif [[ $# -eq 1 && "$1" == "disable" ]]; then

    # remove service symlinks
    if [[ ! -d "${SYSTEM_DBUS_SERVICES}" ]]; then
        echo "Skipping services - doesn't exist"
    elif [[ -L "${SYSTEM_DBUS_SERVICES}" ]]; then
        if [[ $(readlink -f "${SYSTEM_DBUS_SERVICES}") != "${KDE_DBUS_SERVICES}" ]]; then
            echo "Skipping services - exists but is not a symlink"
        else
            sudo rm "${SYSTEM_DBUS_SERVICES}"
            echo "Removed services symlink"
        fi
    else
        echo "Removing service symlinks"
        for kde_service in "${KDE_DBUS_SERVICES}"/*; do
            service=$(basename "$kde_service")
            system_service="${SYSTEM_DBUS_SERVICES}"/"${service}"
            if [[ ! -e "${system_service}" ]]; then
                echo "Skipping ${service} - doesn't exist"
            elif [[ -L "${system_service}" && $(readlink -f "${system_service}") == "${kde_service}" ]]; then
                sudo rm "${system_service}"
                echo "Removed ${service} symlink"
            fi
        done
    fi

    # remove config symlinks
    if [[ ! -d "${SYSTEM_DBUS_CONFIGS}" ]]; then
        echo "Skipping configs - doesn't exist"
    elif [[ -L "${SYSTEM_DBUS_CONFIGS}" ]]; then
        if [[ $(readlink -f "${SYSTEM_DBUS_CONFIGS}") != "${KDE_DBUS_CONFIGS}" ]]; then
            echo "Warning: Skipping configs - exists but is not a symlink"
        else
            sudo rm "${SYSTEM_DBUS_CONFIGS}"
            echo "Removed configs symlink"
        fi
    else
        echo "Removing config symlinks"
        for kde_config in "${KDE_DBUS_CONFIGS}"/*; do
            config=$(basename "$kde_config")
            system_config="${SYSTEM_DBUS_CONFIGS}"/"${config}"
            if [[ ! -e "${system_config}" ]]; then
                echo "Skipping ${config} - doesn't exist"
            elif [[ -L "${system_config}" && $(readlink -f "${system_config}") == "${kde_config}" ]]; then
                sudo rm "${system_config}"
                echo "Removed ${config} symlink"
            fi
        done
    fi

    echo "Done"

else
    usage
    exit 1
fi
