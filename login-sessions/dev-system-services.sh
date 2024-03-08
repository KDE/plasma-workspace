#!/bin/bash
# SPDX-FileCopyrightText: 2023 Jakob Petsovits <jpetso@petsovits.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later

set -e

SYSTEM_DBUS_BASEDIR=/usr/local/share/dbus-1
SYSTEM_DBUS_SERVICES=${SYSTEM_DBUS_BASEDIR}/system-services
SYSTEM_DBUS_CONFIGS=${SYSTEM_DBUS_BASEDIR}/system.d

KDE_DBUS_BASEDIR=/opt/kde-dbus-scripts
KDE_DBUS_SERVICES=${KDE_DBUS_BASEDIR}/system-services
KDE_DBUS_CONFIGS=${KDE_DBUS_BASEDIR}/system.d

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
        echo "Error: ${KDE_DBUS_BASEDIR} doesn't exist - aborting"
        echo "       Use install-sessions.sh at least once to set up the folder structure."
        exit 1
    fi

    # symlink services
    if [[ -L "${SYSTEM_DBUS_SERVICES}" && $(readlink -f "${SYSTEM_DBUS_SERVICES}") == "${KDE_DBUS_SERVICES}" ]]; then
        echo "Skipping services - system services are already symlinked to KDE services"
    else
        echo "Symlinking services ..."
        if [[ ! -d "${SYSTEM_DBUS_SERVICES}" ]]; then
            sudo mkdir -p "${SYSTEM_DBUS_SERVICES}"
            echo "Created system services directory"
        fi
        for kde_service in "${KDE_DBUS_SERVICES}"/*; do
            service=$(basename "$kde_service")
            system_service="${SYSTEM_DBUS_SERVICES}"/"${service}"
            if [[ -e "${system_service}" ]]; then
                sudo mv "${system_service}" "${system_service}~"
                echo "Backed up ${service} - file already existed"
            fi
            sudo ln -s "${kde_service}" "${system_service}"
            echo "Symlinked ${service}"
        done
    fi

    # symlink configs
    if [[ -L "${SYSTEM_DBUS_CONFIGS}" && $(readlink -f "${SYSTEM_DBUS_CONFIGS}") == "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Skipping configs - system configs are already symlinked to KDE configs"
    else
        echo "Symlinking configs ..."
        if [[ ! -d "${SYSTEM_DBUS_CONFIGS}" ]]; then
            sudo mkdir -p "${SYSTEM_DBUS_CONFIGS}"
            echo "Created system configs directory"
        fi
        for kde_config in "${KDE_DBUS_CONFIGS}"/*; do
            config=$(basename "$kde_config")
            system_config="${SYSTEM_DBUS_CONFIGS}"/"${config}"
            if [[ -e "${system_config}" ]]; then
                sudo mv "${system_config}" "${system_config}~"
                echo "Backed up ${service} - file already existed"
            fi
            sudo ln -s "${kde_config}" "${system_config}"
            echo "Symlinked ${config}"
        done
    fi

    echo "Done"

elif [[ $# -eq 1 && "$1" == "disable" ]]; then

    # remove service symlinks and restore existing ones
    if [[ ! -d "${SYSTEM_DBUS_SERVICES}" ]]; then
        echo "Skipping services - system services directory doesn't exist"
    elif [[ -L "${SYSTEM_DBUS_SERVICES}" ]]; then
        if [[ $(readlink -f "${SYSTEM_DBUS_SERVICES}") != "${KDE_DBUS_SERVICES}" ]]; then
            echo "Skipping services - system services are a symlink to something other than KDE services"
        else
            sudo rm "${SYSTEM_DBUS_SERVICES}"
            echo "Removed services symlink"
        fi
    else
        echo "Removing service symlinks ..."
        for kde_service in "${KDE_DBUS_SERVICES}"/*; do
            service=$(basename "$kde_service")
            system_service="${SYSTEM_DBUS_SERVICES}"/"${service}"
            if [[ ! -e "${system_service}" ]]; then
                echo "Skipping ${service} - symlink doesn't exist"
            elif [[ -L "${system_service}" && $(readlink -f "${system_service}") == "${kde_service}" ]]; then
                sudo rm "${system_service}"
                echo "Removed ${service} symlink"
            fi
            if [[ -e "${system_service}~" ]]; then
                sudo mv "${system_service}~" "${system_service}"
                echo "Restored original ${service}"
            fi
        done
    fi

    # remove config symlinks and restore existing ones
    if [[ ! -d "${SYSTEM_DBUS_CONFIGS}" ]]; then
        echo "Skipping configs - system configs directory doesn't exist"
    elif [[ -L "${SYSTEM_DBUS_CONFIGS}" ]]; then
        if [[ $(readlink -f "${SYSTEM_DBUS_CONFIGS}") != "${KDE_DBUS_CONFIGS}" ]]; then
            echo "Skipping configs - system configs are a symlink to something other than KDE configs"
        else
            sudo rm "${SYSTEM_DBUS_CONFIGS}"
            echo "Removed configs symlink"
        fi
    else
        echo "Removing config symlinks ..."
        for kde_config in "${KDE_DBUS_CONFIGS}"/*; do
            config=$(basename "$kde_config")
            system_config="${SYSTEM_DBUS_CONFIGS}"/"${config}"
            if [[ ! -e "${system_config}" ]]; then
                echo "Skipping ${config} - symlink doesn't exist"
            elif [[ -L "${system_config}" && $(readlink -f "${system_config}") == "${kde_config}" ]]; then
                sudo rm "${system_config}"
                echo "Removed ${config} symlink"
            fi

            if [[ -e "${system_config}~" ]]; then
                sudo mv "${system_config}~" "${system_config}"
                echo "Restored original ${config}"
            fi
        done
    fi

    echo "Done"

else
    usage
    exit 1
fi
