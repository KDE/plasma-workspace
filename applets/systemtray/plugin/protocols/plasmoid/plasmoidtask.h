/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef PLASMOIDTASK_H
#define PLASMOIDTASK_H

#include "../../task.h"

#include <Plasma/DataEngine>
#include <Plasma/Containment>

class KIconLoader;
class KJob;
class PlasmoidInterface;

namespace Plasma
{

class Service;
class Applet;
class Contaiment;

}

namespace PlasmaQuick {
    class AppletQuickItem;
}

namespace SystemTray
{

class PlasmoidTaskPrivate;

class PlasmoidTask : public Task
{
    Q_OBJECT

    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut NOTIFY changedShortcut)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)

    Q_PROPERTY(QQuickItem* taskItem READ taskItem NOTIFY taskItemChanged)
    Q_PROPERTY(QQuickItem* taskItemExpanded READ taskItemExpanded NOTIFY taskItemExpandedChanged)

    friend class PlasmoidProtocol;

public:
    PlasmoidTask(const QString &packageName, int appletId, Plasma::Containment *cont, QObject *parent);
    ~PlasmoidTask() override;

    bool isValid() const;
    bool isEmbeddable() const override;
    QString taskId() const override;
    QQuickItem* taskItem() override;
    QQuickItem* taskItemExpanded() override;
    QIcon icon() const override;
    bool isWidget() const override;
    TaskType type() const override { return TypePlasmoid; };
    bool expanded() const override;
    void setExpanded(bool expanded) override;

    QString iconName() const { return m_iconName; }
    KPluginInfo pluginInfo() const;
    QKeySequence shortcut() const;
    void setShortcut(const QKeySequence &sequence);
    Plasma::Applet *applet();

    Q_INVOKABLE void showMenu(int x, int y);

    Q_INVOKABLE void setLocation(Plasma::Types::Location loc);

    Q_INVOKABLE void configure();

Q_SIGNALS:
    void changedShortcut();
    void taskItemChanged();
    void taskItemExpandedChanged();
    void iconNameChanged();

private Q_SLOTS:
    void syncStatus(QString status);

private:
    void updateStatus();
    QString m_taskId;
    Plasma::Applet *m_applet;

    PlasmaQuick::AppletQuickItem* m_taskGraphicsObject;
    QQuickItem* m_compactRepresentationItem;
    QQuickItem* m_fullRepresentationItem;

    QIcon m_icon;
    QString m_iconName;

    bool m_valid;
};

}


#endif
