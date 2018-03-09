/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2007 Christopher Blauvelt <cblauvelt@gmail.com>
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

#ifndef HDDTEMP_H
#define HDDTEMP_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QTimer>


class HddTemp : public QObject
{
    Q_OBJECT

    public:
        enum DataType {Temperature=0, Unit};
        
        explicit HddTemp(QObject *parent = nullptr);
        ~HddTemp() override;
        QStringList sources();
        QVariant data(const QString source, const DataType type) const;

    protected:
        void timerEvent(QTimerEvent *event) override;

    private:
        int m_failCount;
        bool m_cacheValid;
        QMap<QString, QList<QVariant> > m_data;
        bool updateData();
};


#endif
