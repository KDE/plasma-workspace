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

#include "configview.h"

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

class PanelConfigView : public ConfigView
{
    Q_OBJECT

public:
    PanelConfigView(Plasma::Containment *interface, PanelView *panelView, QWindow *parent = 0);
    virtual ~PanelConfigView();

    void init();

    Q_INVOKABLE QAction *action(const QString &name);

public Q_SLOTS:
    void showAddWidgetDialog();

protected:
    void focusOutEvent(QFocusEvent *ev);

protected Q_SLOTS:
    void syncGeometry();

private Q_SLOTS:
    void updateContrast();

private:
    Plasma::Containment *m_containment;
    PanelView *m_panelView;
    Plasma::Theme m_theme;
};

#endif // multiple inclusion guard
