/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/Theme>
#include <PlasmaQuick/PopupPlasmaWindow>
#include <PlasmaQuick/SharedQmlEngine>

#include "panelview.h"

#include <QJSValue>
#include <QPointer>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardItemModel>
#include <plasmaquick/plasmawindow.h>

class PanelView;

namespace Plasma
{
class Containment;
}

class PanelRulerView : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT

public:
    PanelRulerView(Plasma::Containment *interface, PanelView *panelView, PanelConfigView *mainConfigView, QWindow *parent = nullptr);
    ~PanelRulerView() override;

    void syncPanelLocation();

protected:
    void showEvent(QShowEvent *ev) override;
    void focusOutEvent(QFocusEvent *ev) override;

private:
    Plasma::Containment *m_containment;
    PanelView *m_panelView;
    PanelConfigView *m_mainConfigView;
    LayerShellQt::Window *m_layerWindow = nullptr;
};

class PanelConfigView : public PlasmaQuick::PopupPlasmaWindow
{
    Q_OBJECT
    Q_PROPERTY(PanelView::VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)
    Q_PROPERTY(PanelView::OpacityMode opacityMode READ opacityMode WRITE setOpacityMode NOTIFY opacityModeChanged)
    Q_PROPERTY(KSvg::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)
    Q_PROPERTY(PanelRulerView *panelRulerView READ panelRulerView CONSTANT)

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView);
    ~PanelConfigView() override;

    void init();

    PanelView::VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    PanelView::OpacityMode opacityMode() const;
    void setOpacityMode(PanelView::OpacityMode mode);

    KSvg::FrameSvg::EnabledBorders enabledBorders() const;

    PanelRulerView *panelRulerView();

protected:
    void keyPressEvent(QKeyEvent *ev) override;
    void showEvent(QShowEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;
    void focusInEvent(QFocusEvent *ev) override;

public Q_SLOTS:
    void showAddWidgetDialog();
    void addPanelSpacer();

protected Q_SLOTS:
    void syncGeometry();

Q_SIGNALS:
    void visibilityModeChanged();
    void opacityModeChanged();
    void enabledBordersChanged();

private:
    void focusVisibilityCheck(QWindow *focusWindow);

    Plasma::Containment *m_containment;
    QPointer<PanelView> m_panelView;
    QPointer<QWindow> m_focusWindow;
    std::unique_ptr<PanelRulerView> m_panelRulerView;
    KSvg::FrameSvg::EnabledBorders m_enabledBorders = KSvg::FrameSvg::AllBorders;
    Plasma::Theme m_theme;
    QTimer m_screenSyncTimer;
    std::unique_ptr<PlasmaQuick::SharedQmlEngine> m_sharedQmlEngine;
};
