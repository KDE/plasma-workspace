/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CHROMEFINDPROFILE_H
#define CHROMEFINDPROFILE_H

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

#endif // CHROMEFINDPROFILE_H
