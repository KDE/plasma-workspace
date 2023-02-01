/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "../kcms-common_p.h"
#include "../krdb/krdb.h"

#include <KColorScheme>
#include <KConfigGroup>

#include <QColorSpace>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGenericMatrix>
#include <QtMath>

#include "colorsapplicator.h"

float lerp(float a, float b, float f)
{
    return (a * (1.0 - f)) + (b * f);
}

qreal cubeRootOf(qreal num)
{
    return qPow(num, 1.0 / 3.0);
}

qreal cubed(qreal num)
{
    return num * num * num;
}

// a structure representing a colour in the OKlab colour space.
// for tinting, OKlab has some desirable properties:
// - lightness is separated from hue (unlike RGB)
//      so we don't make light themes darkish or dark themes lightish
// - allows accurately adjusting hue without affecting perceptual lightness (unlike HSL/V)
//      so we keep light themes and dark themes at the same perceptual lightness
// - can be linearly blended
//      once we get into oklab, we don't need fancy math to manipulate colours, we can just use a bog-standard
//      linear interpolation function on the a and b values
struct LAB {
    qreal L = 0;
    qreal a = 0;
    qreal b = 0;
};

// precomputed matrices from Björn Ottosson, public domain. or MIT if your country doesn't do that.
/*
    SPDX-FileCopyrightText: 2020 Björn Ottosson

    SPDX-License-Identifier: MIT
    SPDX-License-Identifier: None
*/

LAB linearSRGBToOKLab(const QColor &c)
{
    // convert from srgb to linear lms

    const auto l = 0.4122214708 * c.redF() + 0.5363325363 * c.greenF() + 0.0514459929 * c.blueF();
    const auto m = 0.2119034982 * c.redF() + 0.6806995451 * c.greenF() + 0.1073969566 * c.blueF();
    const auto s = 0.0883024619 * c.redF() + 0.2817188376 * c.greenF() + 0.6299787005 * c.blueF();

    // convert from linear lms to non-linear lms

    const auto l_ = cubeRootOf(l);
    const auto m_ = cubeRootOf(m);
    const auto s_ = cubeRootOf(s);

    // convert from non-linear lms to lab

    return LAB{.L = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_,
               .a = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_,
               .b = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_};
}

QColor OKLabToLinearSRGB(LAB lab)
{
    // convert from lab to non-linear lms

    const auto l_ = lab.L + 0.3963377774 * lab.a + 0.2158037573 * lab.b;
    const auto m_ = lab.L - 0.1055613458 * lab.a - 0.0638541728 * lab.b;
    const auto s_ = lab.L - 0.0894841775 * lab.a - 1.2914855480 * lab.b;

    // convert from non-linear lms to linear lms

    const auto l = cubed(l_);
    const auto m = cubed(m_);
    const auto s = cubed(s_);

    // convert from linear lms to linear srgb

    const auto r = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    const auto g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    const auto b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;

    return QColor::fromRgbF(r, g, b);
}

auto toLinearSRGB = QColorSpace(QColorSpace::SRgb).transformationToColorSpace(QColorSpace::SRgbLinear);
auto fromLinearSRGB = QColorSpace(QColorSpace::SRgbLinear).transformationToColorSpace(QColorSpace::SRgb);

QColor tintColor(const QColor &base, const QColor &with, qreal factor)
{
    auto baseLAB = linearSRGBToOKLab(toLinearSRGB.map(base));
    const auto withLAB = linearSRGBToOKLab(toLinearSRGB.map(with));
    baseLAB.a = lerp(baseLAB.a, withLAB.a, factor);
    baseLAB.b = lerp(baseLAB.b, withLAB.b, factor);

    return fromLinearSRGB.map(OKLabToLinearSRGB(baseLAB));
}

static void copyEntry(KConfigGroup &from, KConfigGroup &to, const QString &entry, KConfig::WriteConfigFlags writeConfigFlag = KConfig::Normal)
{
    if (from.hasKey(entry)) {
        to.writeEntry(entry, from.readEntry(entry), writeConfigFlag);
    }
}

void applyScheme(const QString &colorSchemePath, KConfig *configOutput, KConfig::WriteConfigFlags writeConfigFlag, std::optional<QColor> accentColor)
{
    const auto accent = accentColor.value_or(configOutput->group("General").readEntry("AccentColor", QColor()));

    const auto hasAccent = [configOutput, &accent, accentColor]() {
        if (accent == QColor(Qt::transparent)) {
            return false;
        }

        return configOutput->group("General").hasKey("AccentColor")
            || accentColor.has_value(); // It's obvious that when accentColor.hasValue, it has (non-default/non-transparent) accent. reading configOutput for
                                        // any config is unreliable in this file.
    }();

    // Using KConfig::SimpleConfig because otherwise Header colors won't be
    // rewritten when a new color scheme is loaded.
    KSharedConfigPtr config = KSharedConfig::openConfig(colorSchemePath, KConfig::SimpleConfig);

    const auto applyAccentToTitlebar =
        config->group("General").readEntry("TitlebarIsAccentColored", config->group("General").readEntry("accentActiveTitlebar", false));
    const auto tintAccent = config->group("General").hasKey("TintFactor");
    const auto tintFactor = config->group("General").readEntry<qreal>("TintFactor", DefaultTintFactor);

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

    for (const auto &item : colorSetGroupList) {
        configOutput->deleteGroup(item);

        // Not all color schemes have header colors; in this case we don't want
        // to write out any header color data because then various things will think
        // the color scheme *does* have header colors, which it mostly doesn't, and
        // things will visually break in creative ways
        if (item == QStringLiteral("Colors:Header") && !config->hasGroup(QStringLiteral("Colors:Header"))) {
            continue;
        }

        KConfigGroup sourceGroup(config, item);
        KConfigGroup targetGroup(configOutput, item);

        for (const auto &entry : colorSetKeyList) {
            if (hasAccent) {
                if (accentList.contains(entry)) {
                    targetGroup.writeEntry(entry, accent);
                } else if (tintAccent) {
                    auto base = sourceGroup.readEntry<QColor>(entry, QColor());
                    targetGroup.writeEntry(entry, tintColor(base, accent, tintFactor));
                } else {
                    copyEntry(sourceGroup, targetGroup, entry);
                }
            } else {
                copyEntry(sourceGroup, targetGroup, entry);
            }
        }

        if (item == QStringLiteral("Colors:Selection") && hasAccent) {
            QColor accentbg = accentBackground(accent, config->group("Colors:View").readEntry<QColor>("BackgroundNormal", QColor()));
            for (const auto &entry : {QStringLiteral("BackgroundNormal"), QStringLiteral("BackgroundAlternate")}) {
                targetGroup.writeEntry(entry, accentbg);
            }
            for (const auto &entry : {QStringLiteral("ForegroundNormal"), QStringLiteral("ForegroundInactive")}) {
                targetGroup.writeEntry(entry, accentForeground(accentbg, true));
            }
        }

        if (item == QStringLiteral("Colors:Button") && hasAccent) {
            QColor accentbg = accentBackground(accent, config->group("Colors:Button").readEntry<QColor>("BackgroundNormal", QColor()));
            for (const auto &entry : {QStringLiteral("BackgroundAlternate")}) {
                targetGroup.writeEntry(entry, accentbg);
            }
        }

        if (sourceGroup.hasGroup("Inactive")) {
            sourceGroup = sourceGroup.group("Inactive");
            targetGroup = targetGroup.group("Inactive");

            for (const auto &entry : colorSetKeyList) {
                if (tintAccent) {
                    auto base = sourceGroup.readEntry<QColor>(entry, QColor());
                    targetGroup.writeEntry(entry, tintColor(base, accent, tintFactor));
                } else {
                    copyEntry(sourceGroup, targetGroup, entry, writeConfigFlag);
                }
            }
        }

        // Header accent colouring
        if (item == QStringLiteral("Colors:Header") && hasAccent) {
            const auto windowBackground = config->group("Colors:Window").readEntry<QColor>("BackgroundNormal", QColor());
            const auto accentedWindowBackground = accentBackground(accent, windowBackground);
            const auto inactiveWindowBackground = tintColor(windowBackground, accent, tintFactor);

            if (applyAccentToTitlebar) {
                targetGroup = KConfigGroup(configOutput, item);
                targetGroup.writeEntry("BackgroundNormal", accentedWindowBackground);
                targetGroup.writeEntry("ForegroundNormal", accentForeground(accentedWindowBackground, true));

                targetGroup = targetGroup.group("Inactive");
                targetGroup.writeEntry("BackgroundNormal", inactiveWindowBackground);
                targetGroup.writeEntry("ForegroundNormal", accentForeground(inactiveWindowBackground, false));
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

    if (hasAccent && (tintAccent || applyAccentToTitlebar)) { // Titlebar accent colouring
        const auto windowBackground = config->group("Colors:Window").readEntry<QColor>("BackgroundNormal", QColor());

        if (tintAccent) {
            const auto tintedWindowBackground = tintColor(windowBackground, accent, tintFactor);
            if (!applyAccentToTitlebar) {
                groupWMOut.writeEntry("activeBackground", tintedWindowBackground, writeConfigFlag);
                groupWMOut.writeEntry("activeForeground", accentForeground(tintedWindowBackground, true), writeConfigFlag);
            }
            groupWMOut.writeEntry("inactiveBackground", tintedWindowBackground, writeConfigFlag);
            groupWMOut.writeEntry("inactiveForeground", accentForeground(tintedWindowBackground, false), writeConfigFlag);
        }

        if (applyAccentToTitlebar) {
            const auto accentedWindowBackground = accentBackground(accent, windowBackground);
            groupWMOut.writeEntry("activeBackground", accentedWindowBackground, writeConfigFlag);
            groupWMOut.writeEntry("activeForeground", accentForeground(accentedWindowBackground, true), writeConfigFlag);
        }
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

    bool applyToAlien{true};
    {
        KConfig cfg(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
        KConfigGroup group(configOutput, "General");
        group = KConfigGroup(&cfg, "X11");
        applyToAlien = group.readEntry("exportKDEColors", applyToAlien);
    }
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | (applyToAlien ? KRdbExportColors : 0));
}
