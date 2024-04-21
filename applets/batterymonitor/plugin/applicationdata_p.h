/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QString>

class ApplicationData
{
public:
    void populateApplicationData(const QString &name, QString *prettyName, QString *icon);

private:
    static QHash<QString, QPair<QString, QString>> m_applicationInfo;
};
