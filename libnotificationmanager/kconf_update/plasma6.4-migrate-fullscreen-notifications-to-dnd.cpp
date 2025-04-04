/*
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <iostream>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QStringLiteral>

/**
 * If the user has enabled the "Show over full screen windows" option,
 * this script will migrate that preference by setting the new
 * "Do Not Disturb when full screen windows are present" false.
 *
 * @since 6.4
 */
int main()
{
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QStringLiteral("plasmanotifyrc"), KConfig::SimpleConfig);
    KConfigGroup notificationsGroup(configPtr, QStringLiteral("Notifications"));
    if (!notificationsGroup.exists()) {
        std::cout << "plasmanotifyrc doesn't have a Notifications group. No need to update config." << std::endl;
        return EXIT_SUCCESS;
    }

    // Check if the legacy "Show over full screen windows" option is enabled.
    bool showOverFullScreen = notificationsGroup.readEntry("NormalAlwaysOnTop", false);
    if (!showOverFullScreen) {
        std::cout << "The 'Show over full screen windows' option is not enabled. No need to update config." << std::endl;
        return EXIT_SUCCESS;
    }

    // Remove the old setting.
    notificationsGroup.deleteEntry("NormalAlwaysOnTop");

    // Disable the new setting to preserve the old behavior.
    KConfigGroup dndGroup(configPtr, QStringLiteral("DoNotDisturb"));
    dndGroup.writeEntry("WhenFullscreen", false);

    // Save the changes to the configuration file.
    if (!configPtr->sync()) {
        std::cerr << "Failed to save changes to plasmanotifyrc." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
