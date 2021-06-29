/*
    SPDX-FileCopyrightText: 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
    SPDX-FileCopyrightText: 2005-2007 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
    SolidUiServer(QObject *parent, const QList<QVariant> &);
    ~SolidUiServer() override;

public Q_SLOTS:
    Q_SCRIPTABLE void showPassphraseDialog(const QString &udi, const QString &returnService, const QString &returnObject, uint wId, const QString &appId);

private Q_SLOTS:
    void onPassphraseDialogCompleted(const QString &pass, bool keep);
    void onPassphraseDialogRejected();

private:
    void reparentDialog(QWidget *dialog, WId wId, const QString &appId, bool modal);

    QMap<QString, KPasswordDialog *> m_idToPassphraseDialog;
};