/*
 *  SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "plasmabackground.h"

PlasmaBackground::PlasmaBackground()
{
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &PlasmaBackground::themeChanged);
}

qreal PlasmaBackground::backgroundContrast() const
{
    return m_theme.backgroundContrast();
}

qreal PlasmaBackground::backgroundIntensity() const
{
    return m_theme.backgroundIntensity();
}

qreal PlasmaBackground::backgroundSaturation() const
{
    return m_theme.backgroundSaturation();
}
