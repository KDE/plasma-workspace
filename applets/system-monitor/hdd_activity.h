/*
 *   Copyright (C) 2011, 2012 Shaun Reich <shaun.reich@kdemail.net>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#ifndef HDD_ACTIVITY_HEADER
#define HDD_ACTIVITY_HEADER

#include "ui_hdd_activity-config.h"
#include "applet.h"

#include <QTimer>
#include <QStandardItemModel>
#include <QRegExp>

#include <Plasma/DataEngine>

namespace Plasma {
}

class QGraphicsLinearLayout;

class Hdd_Activity : public SM::Applet
{
    Q_OBJECT
public:
    Hdd_Activity(QObject *parent, const QVariantList &args);
    ~Hdd_Activity();

    virtual void init();

    virtual bool addVisualization(const QString &source);
    virtual void createConfigurationInterface(KConfigDialog *parent);

public Q_SLOTS:
    void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);
    void sourceChanged(const QString &name);
    void sourcesChanged();

    void configAccepted();
    void configChanged();

private:
    Ui::config ui;
    QStandardItemModel m_hddModel;

    /**
     * The hdd sources that would be interesting to watch.
     * Does not necessarily mean they're being watched,
     * sources() indicates that.
     */
    QStringList m_possibleHdds;

    /**
     * For each raw hdd name source, stores
     * reads at 0, writes at 1.
     *
     * Needed because we get those dataUpdated changes
     * separately.
    */
    QMap<QString, QVector<double> > m_data;

    QRegExp m_regexp;
};

K_EXPORT_PLASMA_APPLET(sm_hdd_activity, Hdd_Activity)

#endif
