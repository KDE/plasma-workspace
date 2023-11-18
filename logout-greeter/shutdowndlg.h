/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QScreen>

#include <PlasmaQuick/QuickViewSharedEngine>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

// The confirmation dialog
class KSMShutdownDlg : public PlasmaQuick::QuickViewSharedEngine
{
    Q_OBJECT

public:
    KSMShutdownDlg(QWindow *parent, const QString &defaultAction, QScreen *screen);

    void setWindowed(bool windowed)
    {
        m_windowed = windowed;
    }
    void init(const KPackage::Package &package);
    bool result() const;

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    bool m_windowed = false;
};
