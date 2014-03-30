/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
    BrowserFactory(QObject *parent =0);
    Browser *find(const QString &browserName, QObject *parent = 0);
private:
  Browser *m_previousBrowser;
  QString m_previousBrowserName;
};

#endif // BROWSERFACTORY_H
