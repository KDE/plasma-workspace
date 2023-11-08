/*
    SPDX-FileCopyrightText: 2023 Akseli Lahtinen <akselmo@akselmo.dev>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <iostream>

#include <KConfigGroup>
#include <KSharedConfig>

/**
 * Plasma 5 panels have more settings than Plasma 6 for hiding the panel.
 * This updates the hiding setting to Always Show, unless
 * Plasma 5 panel has setting "Covered By Windows", which is changed to "hiding",
 * or Plasma 5 panel already has hiding setting enabld.
 */

int main()
{
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"), KConfig::FullConfig);
    KConfigGroup views(configPtr, QStringLiteral("PlasmaViews"));
    if (!views.exists() || views.groupList().empty()) {
        std::cout << "plasmashellrc doesn't have any PlasmaViews. No need to update config." << std::endl;
        return EXIT_SUCCESS;
    }

    // Update panelVisibility setting in config groups like [PlasmaViews][Panel 114]
    const QStringList groupList = views.groupList();
    for (const QString &name : groupList) {
        if (!name.startsWith(QLatin1String("Panel "))) {
            continue;
        }

        KConfigGroup panelConfigGroup(&views, name);
        const QString setting = "panelVisibility";
        const QString currentSetting = panelConfigGroup.readEntry(setting);

        // Do nothing if the setting is not set
        if (currentSetting.isEmpty()) {
            continue;
        }
        // Set hiding enabled if covered by windows is enabled
        else if (currentSetting == "2") {
            panelConfigGroup.writeEntry(setting, 1);
            continue;
        }
        // Else if setting is not already hiding, set it to always shown
        else if (currentSetting != "1") {
            panelConfigGroup.writeEntry(setting, 0);
            continue;
        }
    }

    return configPtr->sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
