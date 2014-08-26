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

#include "plasmaquick/configview.h"
#include "panelview.h"

#include <QQuickItem>
#include <QQuickView>
#include <QJSValue>
#include <QQmlListProperty>
#include <QStandardItemModel>
#include <Plasma/Theme>

class AppletInterface;
class ConfigPropertyMap;
class PanelView;

namespace Plasma {
    class Containment;
}

class PanelConfigView : public PlasmaQuick::ConfigView
{
    Q_OBJECT
    Q_PROPERTY(PanelView::VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView, QWindow *parent = 0);
    virtual ~PanelConfigView();

    void init();

    PanelView::VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

protected:
    void showEvent(QShowEvent *ev);
    void hideEvent(QHideEvent *ev);

public Q_SLOTS:
    void showAddWidgetDialog();
    void addPanelSpacer();

protected:
    void focusOutEvent(QFocusEvent *ev);

protected Q_SLOTS:
    void syncGeometry();

private Q_SLOTS:
    void updateContrast();

Q_SIGNALS:
    void visibilityModeChanged();

private:
    Plasma::Containment *m_containment;
    QPointer<PanelView> m_panelView;
    PanelView::VisibilityMode m_visibilityMode;
    Plasma::Theme m_theme;
    QTimer m_deleteTimer;
    QTimer m_screenSyncTimer;
};

#endif // multiple inclusion guard
