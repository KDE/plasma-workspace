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

function ensure_none_or_kde_symlinks()
{
    if [[ -e "${SYSTEM_DBUS_SERVICES}" && $(readlink -f "${SYSTEM_DBUS_SERVICES}") != "${KDE_DBUS_SERVICES}" ]]; then
        echo "Error: ${SYSTEM_DBUS_SERVICES} exists but is not a link to ${KDE_DBUS_SERVICES} - aborting"
        exit 1
    fi
    if [[ -e "${SYSTEM_DBUS_CONFIGS}" && $(readlink -f "${SYSTEM_DBUS_CONFIGS}") != "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Error: ${SYSTEM_DBUS_CONFIGS} exists but is not a link to ${KDE_DBUS_CONFIGS} - aborting"
        exit 1
    fi
}

if [[ $# -eq 1 && ( "$1" == "--help" || "$1" == "-h" ) ]]; then

    usage

elif [[ $# -eq 1 && "$1" == "enable" ]]; then

    if [[ ! -d "${KDE_DBUS_SERVICES}" || ! -d "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Error: ${KDE_DBUS_SERVICES} doesn't exist - aborting"
        echo "       Use install-sessions.sh at least once to set up the folder structure in /opt/kde-dbus-scripts"
        exit 1
    fi
    ensure_none_or_kde_symlinks

    if [[ -L "${SYSTEM_DBUS_SERVICES}" && $(readlink -f "${SYSTEM_DBUS_SERVICES}") == "${KDE_DBUS_SERVICES}" ]]; then
        echo "Skipping ${SYSTEM_DBUS_SERVICES} - already symlinked to ${KDE_DBUS_SERVICES}"
    else
        sudo mkdir -p "${SYSTEM_DBUS_BASEDIR}"
        sudo ln -s "${KDE_DBUS_SERVICES}" "${SYSTEM_DBUS_SERVICES}"
        ls -ld "${SYSTEM_DBUS_SERVICES}"
    fi

    if [[ -L "${SYSTEM_DBUS_CONFIGS}" && $(readlink -f "${SYSTEM_DBUS_CONFIGS}") == "${KDE_DBUS_CONFIGS}" ]]; then
        echo "Skipping ${SYSTEM_DBUS_CONFIGS} - already symlinked to ${KDE_DBUS_CONFIGS}"
    else
        sudo mkdir -p "${SYSTEM_DBUS_BASEDIR}"
        sudo ln -s "${KDE_DBUS_CONFIGS}" "${SYSTEM_DBUS_CONFIGS}"
        ls -ld "${SYSTEM_DBUS_CONFIGS}"
    fi

elif [[ $# -eq 1 && "$1" == "disable" ]]; then

    ensure_none_or_kde_symlinks

    if [[ ! -e "${SYSTEM_DBUS_SERVICES}" ]]; then
        echo "Skipping ${SYSTEM_DBUS_SERVICES} - doesn't exist, can't remove"
    else
        sudo rm "${SYSTEM_DBUS_SERVICES}"
        echo "Removed ${SYSTEM_DBUS_SERVICES} symlink"
    fi

    if [[ ! -e "${SYSTEM_DBUS_CONFIGS}" ]]; then
        echo "Skipping ${SYSTEM_DBUS_CONFIGS} - doesn't exist, can't remove"
    else
        sudo rm "${SYSTEM_DBUS_CONFIGS}"
        echo "Removed ${SYSTEM_DBUS_CONFIGS} symlink"
    fi

else
    usage
    exit 1
fi
