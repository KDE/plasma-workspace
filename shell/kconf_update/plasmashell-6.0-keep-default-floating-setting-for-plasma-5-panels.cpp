/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <iostream>

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

/**
 * For Plasma 5 users, keep the existing panel floating settings.
 *
 * @see https://invent.kde.org/plasma/plasma-desktop/-/issues/73
 * @since 6.0
 */
int main()
{
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"), KConfig::FullConfig);
    KConfigGroup views(configPtr, QStringLiteral("PlasmaViews"));
    if (!views.exists() || views.groupList().empty()) {
        std::cout << "plasmashellrc doesn't have any PlasmaViews. No need to update config." << std::endl;
        return EXIT_SUCCESS;
    }

    // Update default floating setting in config groups like [PlasmaViews][Panel 114][Defaults]
    const QStringList groupList = views.groupList();
    for (const QString &name : groupList) {
        if (!name.startsWith(QLatin1String("Panel "))) {
            continue;
        }

        KConfigGroup panelConfigGroup(&views, name);
        if (!panelConfigGroup.hasGroup(u"Defaults"_s)) {
            continue;
        }

        KConfigGroup defaultConfigGroup(&panelConfigGroup, u"Defaults"_s);
        if (defaultConfigGroup.hasKey("floating")) {
            // Respect the manual setting
            continue;
        }

        // Explicitly set the old default floating setting for panels from Plasma 5
        defaultConfigGroup.writeEntry("floating", 0);
    }

    return configPtr->sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
