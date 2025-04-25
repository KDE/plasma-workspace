/*
 *  SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <qqmlregistration.h>

#include <Plasma/Theme>

class PlasmaBackground : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(qreal backgroundContrast READ backgroundContrast NOTIFY themeChanged)
    Q_PROPERTY(qreal backgroundIntensity READ backgroundIntensity NOTIFY themeChanged)
    Q_PROPERTY(qreal backgroundSaturation READ backgroundSaturation NOTIFY themeChanged)

public:
    PlasmaBackground();

    qreal backgroundContrast() const;
    qreal backgroundIntensity() const;
    qreal backgroundSaturation() const;

Q_SIGNALS:
    void themeChanged();

private:
    Plasma::Theme m_theme;
};
