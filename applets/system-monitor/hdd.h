/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
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

#ifndef HDD_HEADER
#define HDD_HEADER

#include <Plasma/DataEngine>
#include <QHash>
#include <QStandardItemModel>
#include <applet.h>
#include "ui_hdd-config.h"

namespace Plasma {
    class Meter;
}
class MonitorIcon;
class QGraphicsLinearLayout;

class Hdd : public SM::Applet
{
    Q_OBJECT
    public:
        Hdd(QObject *parent, const QVariantList &args);
        ~Hdd();

        virtual void init();
        virtual void createConfigurationInterface(KConfigDialog *parent);

    public Q_SLOTS:
        void configChanged();
        void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);

    protected:
        QString hddTitle(const QString& uuid, const Plasma::DataEngine::Data &data);
        QString guessHddTitle(const Plasma::DataEngine::Data &data);
        QString filePath(const Plasma::DataEngine::Data &data);
        bool addVisualization(const QString& source);
        virtual void deleteVisualizations();
        bool isValidDevice(const QString& uuid, Plasma::DataEngine::Data* data);
        void applyTheme(Plasma::Meter *w);

    protected Q_SLOTS:
        void configAccepted();
        void themeChanged();

    private:
        Ui::config ui;
        QStandardItemModel m_hddModel;
        QHash<const QString, MonitorIcon *> m_icons;
        QHash<const QString, QGraphicsLinearLayout *> m_layouts;
        QHash<QString, QList<Plasma::Meter *> > m_diskMap;
        QStringList mounted();
};

K_EXPORT_PLASMA_APPLET(sm_hdd, Hdd)

#endif
