/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QQuickWindow>

#include <KScreenDpms/Dpms>

class QQuickWindow;

/**
 * This class monitors if monitors are turned off
 */
class DPMSMonitor : public KScreen::Dpms
{
    Q_OBJECT

    Q_PROPERTY(bool monitorOn READ monitorOn NOTIFY monitorOnChanged)
    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit DPMSMonitor(QObject *parent = nullptr);
    ~DPMSMonitor() override;

    bool monitorOn() const;

    QQuickWindow *window() const;
    void setWindow(QQuickWindow *window);

Q_SIGNALS:
    void monitorOnChanged();
    void windowChanged();

public Q_SLOTS:
    void onScreenChanged(QScreen *screen);
    void onModeChanged(KScreen::Dpms::Mode mode, QScreen *screen);

private:
    bool m_monitorOn = true;
    QQuickWindow *m_window = nullptr;
    QScreen *m_screen = nullptr;
};
