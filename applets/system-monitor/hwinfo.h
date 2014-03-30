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

#ifndef HWINFO_HEADER
#define HWINFO_HEADER

#include <applet.h>
#include <Plasma/DataEngine>

namespace Plasma {
    class TextBrowser;
    class IconWidget;
}

class HWInfo : public SM::Applet
{
    Q_OBJECT
    public:
        HWInfo(QObject *parent, const QVariantList &args);
        ~HWInfo();

        virtual void init();
        virtual bool addVisualization(const QString&);

    public Q_SLOTS:
        void dataUpdated(const QString &name,
                         const Plasma::DataEngine::Data &data);

    private Q_SLOTS:
        void updateHtml();

    protected:
        virtual void deleteVisualizations();

    private:
        void setSources();

        Plasma::TextBrowser *m_info;
        Plasma::IconWidget *m_icon;
        QString m_gpu;
        QStringList m_cpus;
        QStringList m_cpuNames;
        QStringList m_networks;
        QStringList m_networkNames;
        QStringList m_audios;
        QStringList m_audioNames;
};

K_EXPORT_PLASMA_APPLET(sm_hwinfo, HWInfo)

#endif
