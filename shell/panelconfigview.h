/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PANELCONFIGVIEW_H
#define PANELCONFIGVIEW_H

#include <PlasmaQuick/ConfigView>
#include <Plasma/Theme>

#include "panelview.h"

#include <QQuickItem>
#include <QQuickView>
#include <QJSValue>
#include <QQmlListProperty>
#include <QStandardItemModel>
#include <QPointer>

class PanelView;

namespace Plasma {
    class Containment;
}

namespace KWayland
{
    namespace Client
    {
        class PlasmaShellSurface;
    }
}

class PanelConfigView : public PlasmaQuick::ConfigView
{
    Q_OBJECT
    Q_PROPERTY(PanelView::VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)
    Q_PROPERTY(Plasma::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView, QWindow *parent = nullptr);
    ~PanelConfigView() override;

    void init() override;

    PanelView::VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    Plasma::FrameSvg::EnabledBorders enabledBorders() const;

protected:
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;
    void moveEvent(QMoveEvent *ev) override;
    bool event(QEvent *e) override;

public Q_SLOTS:
    void showAddWidgetDialog();
    void addPanelSpacer();

protected Q_SLOTS:
    void syncGeometry();
    void syncLocation();

private Q_SLOTS:
    void updateBlurBehindAndContrast();

Q_SIGNALS:
    void visibilityModeChanged();
    void enabledBordersChanged();

private:
    Plasma::Containment *m_containment;
    QPointer<PanelView> m_panelView;
    Plasma::FrameSvg::EnabledBorders m_enabledBorders = Plasma::FrameSvg::AllBorders;
    Plasma::Theme m_theme;
    QTimer m_screenSyncTimer;
    QPointer<KWayland::Client::PlasmaShellSurface> m_shellSurface;
};

#endif // multiple inclusion guard
