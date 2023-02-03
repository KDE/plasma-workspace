/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <iostream>
#include <optional>

#include <KConfigGroup>
#include <KSharedConfig>

/**
 * plasmashellrc have 3 thickness values in "Default" "Horizontal" "Vertical"
 * groups. Only the value in the default group is preserved to avoid desync
 * between horizontal and vertical panels.
 *
 * @see BUG 460006
 * @since 5.27
 */
int main()
{
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QString::fromLatin1("plasmashellrc"), KConfig::FullConfig);
    KConfigGroup views(configPtr, QStringLiteral("PlasmaViews"));
    if (!views.exists() || views.groupList().empty()) {
        std::cout << "plasmashellrc doesn't have any PlasmaViews. No need to update config." << std::endl;
        return EXIT_SUCCESS;
    }

    // Find config groups like [PlasmaViews][Panel 114][Horizontal1536]
    // or [PlasmaViews][Panel 115][Vertical864]
    unsigned count = 0;
    const QStringList groupList = views.groupList();
    for (const QString &name : groupList) {
        if (!name.startsWith(QLatin1String("Panel"))) {
            continue;
        }

        KConfigGroup panelConfigGroup(&views, name);
        const QStringList groupList = panelConfigGroup.groupList();
        std::optional<unsigned> thickness;
        for (const QString &name : groupList) {
            if (!name.startsWith(QLatin1String("Horizontal")) && !name.startsWith(QLatin1String("Vertical"))) {
                continue;
            }

            KConfigGroup panelFormConfigGroup(&panelConfigGroup, name);
            if (!panelFormConfigGroup.hasKey("thickness")) {
                continue;
            }

            const unsigned valueInGroup = panelFormConfigGroup.readEntry("thickness", unsigned(0));
            if (!thickness.has_value() || thickness.value() > valueInGroup) {
                thickness = valueInGroup; // Use the minimum thickness value
            }

            panelFormConfigGroup.deleteEntry("thickness");
            ++count;
            // Should have at least one default group so no need to delete the whole panel group
            if (panelFormConfigGroup.keyList().empty()) {
                panelConfigGroup.deleteGroup(name);
            }
        }

        // Update the thickness in [Defaults] group
        if (thickness.has_value()) {
            KConfigGroup defaultConfigGroup(&panelConfigGroup, "Defaults");
            defaultConfigGroup.writeEntry("thickness", thickness.value());
        }
    }

    if (count == 0) {
        std::cout << "No need to update config." << std::endl;
    } else {
        configPtr->sync();
    }

    return EXIT_SUCCESS;
}
