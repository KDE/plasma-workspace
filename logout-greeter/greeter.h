/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QList>
#include <QObject>

#include <kworkspace.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

class KSMShutdownDlg;

class QScreen;

class Greeter : public QObject
{
    Q_OBJECT
public:
    Greeter(const KPackage::Package &package);
    ~Greeter() override;

    void init();
    void enableWindowed();

public Q_SLOTS:
    void promptLogout();
    void promptShutDown();
    void promptReboot();

private:
    void adoptScreen(QScreen *screen);

    bool m_running = false;

    QString m_defaultAction;
    QList<KSMShutdownDlg *> m_dialogs;
    bool m_windowed = false;
    KPackage::Package m_package;
};
