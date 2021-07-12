/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browsers/findprofile.h"
#include <QDir>
#include <QObject>

class FindChromeProfile : public QObject, public FindProfile
{
public:
    explicit FindChromeProfile(const QString &applicationName, const QString &homeDirectory = QDir::homePath(), QObject *parent = nullptr);
    QList<Profile> find() override;

private:
    QString const m_applicationName;
    QString const m_homeDirectory;
};
