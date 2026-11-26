// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Peder Bergan <pederbe@users.noreply.github.com>

#pragma once

#include <Plasma/Theme>
#include <qqmlregistration.h>

class LauncherTheme : public Plasma::Theme
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QColor textColor READ textColor NOTIFY themeChanged FINAL)

public:
    explicit LauncherTheme(QObject *parent = nullptr)
        : Plasma::Theme(parent)
    {
    }

    QColor textColor() const
    {
        return color(Plasma::Theme::TextColor);
    }
};
