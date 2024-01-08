/*
    SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <iostream>

#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>

using namespace Qt::StringLiterals;

/**
 * If the user had specified a custom position for its panel, then
 * set "Custom" as the panel length mode; this will make the panel
 * preserve location and size in Plasma 6.
 *
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
        if (!panelConfigGroup.hasKey("panelLengthMode") && (defaultConfigGroup.hasKey("maxLength") || defaultConfigGroup.hasKey("minLength"))) {
            // Since this panel has a custom size, we will
            // set the custom size length mode flag.
            panelConfigGroup.writeEntry("panelLengthMode", 2);
        }
    }

    return configPtr->sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
