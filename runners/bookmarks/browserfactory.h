/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include <QObject>
#include <QString>

class Browser;
class BrowserFactory : public QObject
{
    Q_OBJECT
public:
    explicit BrowserFactory(QObject *parent = nullptr);
    Browser *find(const QString &browserName, QObject *parent = nullptr);

private:
    Browser *m_previousBrowser;
    QString m_previousBrowserName;
};
