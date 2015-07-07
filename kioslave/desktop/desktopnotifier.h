/* This file is part of the KDE Project
   Copyright (C) 2008, 2009 Fredrik Höglund <fredrik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DESKTOPNOTIFIER_H
#define DESKTOPNOTIFIER_H

#include <kdedmodule.h>
#include <QtDBus/QtDBus>

class KDirWatch;

class DesktopNotifier : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.DesktopNotifier")

public:
    DesktopNotifier(QObject* parent, const QList<QVariant>&);

public slots:
    Q_SCRIPTABLE Q_NOREPLY void watchDir(const QString &path);

private slots:
    void dirty(const QString &path);

private:
    KDirWatch *dirWatch;
};

#endif

