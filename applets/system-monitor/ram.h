/*
 *   Copyright (C) 2008 Petri Damsten <damu@iki.fi>
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

#ifndef RAM_HEADER
#define RAM_HEADER

#include <ui_ram-config.h>
#include "applet.h"
#include <Plasma/DataEngine>
#include <QStandardItemModel>

class QStandardItemModel;

namespace SM {

class Ram : public Applet
{
    Q_OBJECT
    public:
        Ram(QObject *parent, const QVariantList &args);
        ~Ram();

        virtual void init();
        virtual bool addVisualization(const QString&);
        virtual void createConfigurationInterface(KConfigDialog *parent);

    public Q_SLOTS:
        void dataUpdated(const QString &name,
                         const Plasma::DataEngine::Data &data);
        void sourceAdded(const QString &name);
        void sourcesAdded();
        void configAccepted();
        void configChanged();

    private:
        // below methods exist because KLocale has no nice
        // way of getting this info :(
        // thought about adding it to the api, but perhaps this
        // code is the only one that uses it?
        /**
         * The preferred binary unit byte value
         * e.g. KiB, kiB, KIB, etc.
         * @return double 1024 or 1000
         */
        double preferredBinaryUnit();

        /**
         * The preferred binary unit abbreviations.
         * @return QStringList B, KiB, MiB, GiB, TiB.\
         * or whatever is best fit for current binary unit
         * settings via klocale.
         */
        QStringList preferredUnitsList();

        Ui::config ui;
        QStandardItemModel m_model;
        QStringList m_memories;
        QHash<QString, double> m_max;
};
}

K_EXPORT_PLASMA_APPLET(sm_ram, SM::Ram)

#endif
