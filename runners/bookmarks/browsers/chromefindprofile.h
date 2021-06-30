/*
    SPDX-FileCopyrightText: 2007, 2012 Glenn Ergeerts <glenn.ergeerts@telenet.be>

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
