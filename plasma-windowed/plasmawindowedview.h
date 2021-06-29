/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "plasmawindowedcorona.h"
#include <PlasmaQuick/ConfigView>
#include <QPointer>
#include <QQuickView>

class KStatusNotifierItem;

class PlasmaWindowedView : public QQuickView
{
    Q_OBJECT

public:
    explicit PlasmaWindowedView(QWindow *parent = nullptr);
    ~PlasmaWindowedView() override;

    void setApplet(Plasma::Applet *applet);
    void setHasStatusNotifier(bool stay);

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void moveEvent(QMoveEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

protected Q_SLOTS:
    void showConfigurationInterface(Plasma::Applet *applet);
    void minimumWidthChanged();
    void minimumHeightChanged();
    void maximumWidthChanged();
    void maximumHeightChanged();

private:
    void updateSniIcon();
    void updateSniTitle();
    void updateSniStatus();

    Plasma::Applet *m_applet;
    QPointer<QObject> m_layout;
    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QQuickItem> m_rootObject;
    QPointer<QQuickItem> m_appletInterface;
    QPointer<KStatusNotifierItem> m_statusNotifier;
    bool m_withStatusNotifier;
};
