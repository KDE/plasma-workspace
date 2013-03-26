/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WIDGETEXPLORERVIEW_H
#define WIDGETEXPLORERVIEW_H

#include <QQuickView>
#include "widgetexplorer.h"

namespace Plasma {
    class Containment;
}


class WidgetExplorerView : public QQuickView
{
    Q_OBJECT

public:
    explicit WidgetExplorerView(const QString &qmlPath, QWindow *parent = 0);
    virtual ~WidgetExplorerView();

    virtual void init();
    void setContainment(Plasma::Containment* c);

private Q_SLOTS:
    void widgetExplorerClosed(bool visible);
    void widgetExplorerStatusChanged();

private:
    Plasma::Containment* m_containment;
    WidgetExplorer* m_widgetExplorer;
    QString m_qmlPath;
};

#endif // WIDGETEXPLORERVIEW_H
