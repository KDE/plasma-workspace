/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
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

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QObject>
#include <QHash>

#include <kactivities/info.h>
#include <kconfiggroup.h>

class QSize;
class QString;
class QPixmap;
class KConfig;

namespace KActivities
{
    class Consumer;
} // namespace KActivities


namespace Plasma
{
    class Corona;
} // namespace Plasma

class DesktopCorona;

/**
 * This class represents one activity.
 * an activity has an ID and a name, from nepomuk.
 * it also is associated with one or more containments.
 *
 * do NOT construct these yourself; use DesktopCorona::activity()
 */
class Activity : public QObject
{
    Q_OBJECT
public:
    Activity(const QString &id, Plasma::Corona *parent = 0);
    ~Activity();

    QString id();
    QString name();
    QPixmap pixmap(const QSize &size); //FIXME do we want diff. sizes? updates?

    enum State {
        Invalid  = KActivities::Info::Invalid,
        Running  = KActivities::Info::Running,
        Starting = KActivities::Info::Starting,
        Stopped  = KActivities::Info::Stopped,
        Stopping = KActivities::Info::Stopping,
        PreCreation = 32
    };

    /**
     * whether this is the currently active activity
     */
    bool isCurrent();

    /**
     * state of the activity
     */
    KActivities::Info::State state();

    /**
     * set the plugin to use when creating new containments
     */
    void setDefaultPlugin(const QString &plugin);

    /**
     * The plugin to use when creating new containments
     */
    QString defaultPlugin() const;

    /**
     * @returns the info object for this activity
     */
    const KActivities::Info * info() const;

    KConfigGroup config() const;

Q_SIGNALS:
    void infoChanged();
    void stateChanged();
    void currentStatusChanged();

    void removed();
    void opened();
    void closed();

public Q_SLOTS:
    void setName(const QString &name);
    void setIcon(const QString &icon);

    /**
     * delete the activity forever
     */
    void remove();

    /**
     * make this activity the current activity
     */
    void activate();

    /**
     * save and remove all our containments
     */
    void close();

    /**
     * load the saved containment(s) for this activity
     */
    void open();

private Q_SLOTS:
    void activityChanged();
    void checkIfCurrent();
    void cleanupActivity();

private:
    QString m_id;
    QString m_name;
    QString m_icon;
    QString m_plugin;
    KActivities::Info *m_info;
    KActivities::Consumer *m_activityConsumer;
    bool m_current;
};

#endif
