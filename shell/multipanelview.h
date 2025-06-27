/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>

#include <PlasmaQuick/QuickViewSharedEngine>
#include <QPointer>
#include <QQuickView>

#include <Plasma/Corona>

#include "config-workspace.h"
#include "containmentconfigview.h"

namespace LayerShellQt
{
class Window;
}

class MultiPanelView : public PlasmaQuick::QuickViewSharedEngine
{
    Q_OBJECT

    /**
     * information about the screen in which the panel is in
     */
    Q_PROPERTY(QScreen *screenToFollow READ screenToFollow WRITE setScreenToFollow NOTIFY screenToFollowChanged)
    Q_PROPERTY(QList<QQuickItem *> containments READ containmentGraphicItems NOTIFY containmentsChanged)

public:
    explicit MultiPanelView(Plasma::Corona *corona, QScreen *targetScreen = nullptr);
    ~MultiPanelView() override;

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen *screen);
    QScreen *screenToFollow() const;

    void adaptToScreen();
    void showEvent(QShowEvent *) override;

    void addContainemt(Plasma::Containment *containment);
    bool removeContainment(Plasma::Containment *containment);
    QList<Plasma::Containment *> containments() const;
    QList<QQuickItem *> containmentGraphicItems() const;

    Plasma::Corona *corona() const;

    Q_INVOKABLE QString fileFromPackage(const QString &key, const QString &fileName);

    void showConfigurationInterface(Plasma::Applet *applet);

    // TODO: change name
    void screenGeometryChanged();

Q_SIGNALS:
    void containmentsChanged();
    void geometryChanged();
    void screenToFollowChanged(QScreen *screen);

private:
    QList<Plasma::Containment *> m_containments;
    QPointer<Plasma::Corona> m_corona;
    QPointer<QScreen> m_screenToFollow;
};
