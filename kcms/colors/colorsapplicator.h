/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <optional>

#include <QColor>
#include <QString>

#include <KColorUtils>
#include <KConfig>

inline QColor alphaBlend(const QColor &foreground, const QColor &background)
{
    const auto foregroundAlpha = foreground.alphaF();
    const auto inverseForegroundAlpha = 1.0 - foregroundAlpha;
    const auto backgroundAlpha = background.alphaF();

    if (foregroundAlpha == 0.0) {
        return background;
    }

    if (backgroundAlpha == 1.0) {
        return QColor::fromRgb((foregroundAlpha * foreground.red()) + (inverseForegroundAlpha * background.red()),
                               (foregroundAlpha * foreground.green()) + (inverseForegroundAlpha * background.green()),
                               (foregroundAlpha * foreground.blue()) + (inverseForegroundAlpha * background.blue()),
                               0xff);
    } else {
        const auto inverseBackgroundAlpha = (backgroundAlpha * inverseForegroundAlpha);
        const auto finalAlpha = foregroundAlpha + inverseBackgroundAlpha;
        Q_ASSERT(finalAlpha != 0.0);

        return QColor::fromRgb((foregroundAlpha * foreground.red()) + (inverseBackgroundAlpha * background.red()),
                               (foregroundAlpha * foreground.green()) + (inverseBackgroundAlpha * background.green()),
                               (foregroundAlpha * foreground.blue()) + (inverseBackgroundAlpha * background.blue()),
                               finalAlpha);
    }
}

inline QColor accentBackground(const QColor &accent, const QColor &background)
{
    auto c = accent;
    c.setAlphaF(0.7);
    return alphaBlend(c, background);
}

inline QColor accentForeground(const QColor &accent, const bool &isActive)
{
    auto c = QColor(Qt::white);
    // light bg
    if (KColorUtils::luma(accent) > 0.5) {
        c = QColor(Qt::black);
    } else {
        // dark bg
        c = QColor(Qt::white);
    }

    if (isActive) {
        c.setAlphaF(1.0);
    } else {
        c.setAlphaF(0.6);
    }
    return alphaBlend(c, accent);
}

/**
 * Performs the task of actually applying a color scheme to the current session, based on
 * color scheme file path and configuration file.
 * When using this function, you select the scheme to use by setting the model's selected scheme
 * @param colorFilePath The scheme color file path
 * @param configOut The config which holds the information on which scheme is currently selected, and what colors it contains
 */
void applyScheme(const QString &colorSchemePath,
                 KConfig *configOut,
                 KConfig::WriteConfigFlags writeFlags = KConfig::Normal,
                 std::optional<QColor> accentColor = std::nullopt);

const qreal DefaultTintFactor = 0.15;

QColor tintColor(const QColor &base, const QColor &with, qreal factor = DefaultTintFactor);
