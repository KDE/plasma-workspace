/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005-2007 Kevin Ottens <ervin@kde.org>

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

#ifndef SOLIDUISERVER_H
#define SOLIDUISERVER_H

#include <kdedmodule.h>
#include <qwindowdefs.h>

#include <QMap>

class DeviceActionsDialog;
class KPasswordDialog;
class QWidget;

class SolidUiServer : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.SolidUiServer")

public:
    SolidUiServer(QObject* parent, const QList<QVariant>&);
    ~SolidUiServer() override;

public Q_SLOTS:
    Q_SCRIPTABLE void showPassphraseDialog(const QString &udi,
                                           const QString &returnService, const QString &returnObject,
                                           uint wId, const QString &appId);


private Q_SLOTS:
    void onPassphraseDialogCompleted(const QString &pass, bool keep);
    void onPassphraseDialogRejected();

private:
    void reparentDialog(QWidget *dialog, WId wId, const QString &appId, bool modal);

    QMap<QString, KPasswordDialog*> m_idToPassphraseDialog;
};
#endif
