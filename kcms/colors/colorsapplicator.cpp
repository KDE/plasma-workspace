/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "../kcms-common_p.h"
#include "../krdb/krdb.h"

#include <KColorScheme>
#include <KConfigGroup>

#include <QDBusConnection>
#include <QDBusMessage>

#include "colorsapplicator.h"

static void copyEntry(KConfigGroup &from, KConfigGroup &to, const QString &entry, KConfig::WriteConfigFlags writeConfigFlag = KConfig::Normal)
{
    if (from.hasKey(entry)) {
        to.writeEntry(entry, from.readEntry(entry), writeConfigFlag);
    }
}

void applyScheme(const QString &colorSchemePath, KConfig *configOutput, KConfig::WriteConfigFlags writeConfigFlag)
{
    KSharedConfigPtr globalConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    globalConfig->sync();

    const auto hasAccent = [globalConfig]() {
        return globalConfig->group("General").hasKey("AccentColor");
    };
    const auto getAccent = [globalConfig]() {
        return globalConfig->group("General").readEntry<QColor>("AccentColor", QColor());
    };

    // Using KConfig::SimpleConfig because otherwise Header colors won't be
    // rewritten when a new color scheme is loaded.
    KSharedConfigPtr config = KSharedConfig::openConfig(colorSchemePath, KConfig::SimpleConfig);

    const QStringList colorSetGroupList{QStringLiteral("Colors:View"),
                                        QStringLiteral("Colors:Window"),
                                        QStringLiteral("Colors:Button"),
                                        QStringLiteral("Colors:Selection"),
                                        QStringLiteral("Colors:Tooltip"),
                                        QStringLiteral("Colors:Complementary"),
                                        QStringLiteral("Colors:Header")};

    const QStringList colorSetKeyList{QStringLiteral("BackgroundNormal"),
                                      QStringLiteral("BackgroundAlternate"),
                                      QStringLiteral("ForegroundNormal"),
                                      QStringLiteral("ForegroundInactive"),
                                      QStringLiteral("ForegroundActive"),
                                      QStringLiteral("ForegroundLink"),
                                      QStringLiteral("ForegroundVisited"),
                                      QStringLiteral("ForegroundNegative"),
                                      QStringLiteral("ForegroundNeutral"),
                                      QStringLiteral("ForegroundPositive"),
                                      QStringLiteral("DecorationFocus"),
                                      QStringLiteral("DecorationHover")};

    const QStringList accentList{QStringLiteral("ForegroundActive"),
                                 QStringLiteral("ForegroundLink"),
                                 QStringLiteral("DecorationFocus"),
                                 QStringLiteral("DecorationHover")};

    for (auto item : colorSetGroupList) {
        configOutput->deleteGroup(item);

        KConfigGroup sourceGroup(config, item);
        KConfigGroup targetGroup(configOutput, item);

        for (const auto &entry : colorSetKeyList) {
            if (hasAccent() && accentList.contains(entry)) {
                targetGroup.writeEntry(entry, getAccent());
            } else {
                copyEntry(sourceGroup, targetGroup, entry);
            }
        }

        if (item == QStringLiteral("Colors:Selection") && hasAccent()) {
            for (const auto& entry : {QStringLiteral("BackgroundNormal"), QStringLiteral("BackgroundAlternate")}) {
                targetGroup.writeEntry(entry, accentBackground(getAccent(), config->group("Colors:View").readEntry<QColor>("BackgroundNormal", QColor())));
            }
        }

        if (sourceGroup.hasGroup("Inactive")) {
            sourceGroup = sourceGroup.group("Inactive");
            targetGroup = targetGroup.group("Inactive");

            for (const auto &entry : colorSetKeyList) {
                copyEntry(sourceGroup, targetGroup, entry, writeConfigFlag);
            }
        }
    }

    KConfigGroup groupWMTheme(config, "WM");
    KConfigGroup groupWMOut(configOutput, "WM");
    KColorScheme inactiveHeaderColorScheme(QPalette::Inactive, KColorScheme::Header, config);

    const QStringList colorItemListWM{QStringLiteral("activeBackground"),
                                      QStringLiteral("activeForeground"),
                                      QStringLiteral("inactiveBackground"),
                                      QStringLiteral("inactiveForeground"),
                                      QStringLiteral("activeBlend"),
                                      QStringLiteral("inactiveBlend")};

    const QVector<QColor> defaultWMColors{KColorScheme(QPalette::Normal, KColorScheme::Header, config).background().color(),
                                          KColorScheme(QPalette::Normal, KColorScheme::Header, config).foreground().color(),
                                          inactiveHeaderColorScheme.background().color(),
                                          inactiveHeaderColorScheme.foreground().color(),
                                          KColorScheme(QPalette::Normal, KColorScheme::Header, config).background().color(),
                                          inactiveHeaderColorScheme.background().color()};

    int i = 0;
    for (const QString &coloritem : colorItemListWM) {
        groupWMOut.writeEntry(coloritem, groupWMTheme.readEntry(coloritem, defaultWMColors.value(i)), writeConfigFlag);
        ++i;
    }

    const QStringList groupNameList{QStringLiteral("ColorEffects:Inactive"), QStringLiteral("ColorEffects:Disabled")};

    const QStringList effectList{QStringLiteral("Enable"),
                                 QStringLiteral("ChangeSelectionColor"),
                                 QStringLiteral("IntensityEffect"),
                                 QStringLiteral("IntensityAmount"),
                                 QStringLiteral("ColorEffect"),
                                 QStringLiteral("ColorAmount"),
                                 QStringLiteral("Color"),
                                 QStringLiteral("ContrastEffect"),
                                 QStringLiteral("ContrastAmount")};

    for (const QString &groupName : groupNameList) {
        KConfigGroup groupEffectOut(configOutput, groupName);
        KConfigGroup groupEffectTheme(config, groupName);

        for (const QString &effect : effectList) {
            groupEffectOut.writeEntry(effect, groupEffectTheme.readEntry(effect), writeConfigFlag);
        }
    }

    configOutput->sync();

    bool applyToAlien{true};
    {
        KConfig cfg(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
        KConfigGroup group(configOutput, "General");
        group = KConfigGroup(&cfg, "X11");
        applyToAlien = group.readEntry("exportKDEColors", applyToAlien);
    }
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | (applyToAlien ? KRdbExportColors : 0));

    notifyKcmChange(GlobalChangeType::PaletteChanged);
}
