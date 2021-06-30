/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <kquickaddons/quickviewsharedengine.h>
#include <kworkspace.h>
#include <sessionmanagement.h>

// The confirmation dialog
class KSMShutdownDlg : public KQuickAddons::QuickViewSharedEngine
{
    Q_OBJECT

public:
    KSMShutdownDlg(QWindow *parent, KWorkSpace::ShutdownType sdtype);

    void init();
    bool result() const;

public Q_SLOTS:
    void accept();
    void reject();
    void slotLogout();
    void slotHalt();
    void slotReboot();
    void slotReboot(int);
    void slotSuspend(int);
    void slotLockScreen();

Q_SIGNALS:
    void accepted();
    void rejected();

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    QString m_bootOption;
    QStringList rebootOptions;
    bool m_result : 1;
    SessionManagement m_session;
};
