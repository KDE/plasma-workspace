/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/Theme>
#include <PlasmaQuick/ConfigView>

#include "panelview.h"

#include <QJSValue>
#include <QPointer>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardItemModel>

class PanelView;

namespace Plasma
{
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
    Q_PROPERTY(PanelView::OpacityMode opacityMode READ opacityMode WRITE setOpacityMode NOTIFY opacityModeChanged)
    Q_PROPERTY(Plasma::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView, QWindow *parent = nullptr);
    ~PanelConfigView() override;

    void init() override;

    PanelView::VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    PanelView::OpacityMode opacityMode() const;
    void setOpacityMode(PanelView::OpacityMode mode);

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
    void opacityModeChanged();
    void enabledBordersChanged();

private:
    Plasma::Containment *m_containment;
    QPointer<PanelView> m_panelView;
    Plasma::FrameSvg::EnabledBorders m_enabledBorders = Plasma::FrameSvg::AllBorders;
    Plasma::Theme m_theme;
    QTimer m_screenSyncTimer;
    QPointer<KWayland::Client::PlasmaShellSurface> m_shellSurface;
};
