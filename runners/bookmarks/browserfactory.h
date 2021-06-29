/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef BROWSERFACTORY_H
#define BROWSERFACTORY_H
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

#endif // BROWSERFACTORY_H
