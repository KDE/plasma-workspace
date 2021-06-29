/*
    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KService>

#include <QCommandLineParser>

class KCMInit : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KCMInit")
public Q_SLOTS: // dbus
    Q_SCRIPTABLE void runPhase1();

public:
    explicit KCMInit(const QCommandLineParser &args);
    ~KCMInit() override;

private:
    bool runModule(const QString &libName, KService::Ptr service);
    void runModules(int phase);
    KService::List m_list;
    QStringList m_alreadyInitialized;
};
