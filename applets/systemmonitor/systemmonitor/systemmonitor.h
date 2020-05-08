/***************************************************************************
 *   Copyright (C) 2019 Marco Martin <mart@kde.org>                        *
 *
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

#pragma once

#include <QPointer>
#include <QStandardItemModel>
#include <Plasma/Applet>

#include <KDesktopFile>
#include <KPackage/Package>

class ApplicationListModel;
class QQuickItem;

namespace KSysGuard {
    class SensorFace;
    class SensorFaceController;
}

class KConfigLoader;


class SystemMonitor : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(KSysGuard::SensorFaceController *faceController READ faceController CONSTANT)

public:
    SystemMonitor( QObject *parent, const QVariantList &args );
    ~SystemMonitor() override;

    void init() override;

    KSysGuard::SensorFaceController *faceController() const;

public Q_SLOTS:
    void configChanged() override;

private:
    KSysGuard::SensorFaceController *m_sensorFaceController = nullptr;
    QString m_pendingStartupPreset;
};

