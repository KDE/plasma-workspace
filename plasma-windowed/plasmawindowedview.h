/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef PLASMAWINDOWEDVIEW_H
#define PLASMAWINDOWEDVIEW_H

#include <QQuickView>
#include <QPointer>
#include "plasmawindowedcorona.h"
#include "plasmaquick/configview.h"

class KStatusNotifierItem;

class PlasmaWindowedView : public QQuickView
{
    Q_OBJECT

public:
    PlasmaWindowedView(QWindow *parent = 0);
    ~PlasmaWindowedView() override;

    void setApplet(Plasma::Applet *applet);
    void setHasStatusNotifier(bool stay);

protected:
    void resizeEvent(QResizeEvent * ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void moveEvent(QMoveEvent *ev) override;
    void hideEvent(QHideEvent *ev) override;

protected Q_SLOTS:
    void showConfigurationInterface(Plasma::Applet *applet);
    void minimumWidthChanged();
    void minimumHeightChanged();
    void maximumWidthChanged();
    void maximumHeightChanged();


private:
    Plasma::Applet *m_applet;
    QPointer<QObject> m_layout;
    QPointer<PlasmaQuick::ConfigView> m_configView;
    KStatusNotifierItem* m_statusNotifier;
    bool m_withStatusNotifier;
};

#endif
