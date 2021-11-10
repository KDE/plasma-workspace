/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Applet>
#include <QPointer>
#include <QStandardItemModel>

#include <KDesktopFile>
#include <KPackage/Package>

#include <faces/SensorFaceController.h>

class ApplicationListModel;
class QQuickItem;

namespace KSysGuard
{
class SensorFace;
}

class KConfigLoader;

class SystemMonitor : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(KSysGuard::SensorFaceController *faceController READ faceController CONSTANT)

public:
    SystemMonitor(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~SystemMonitor() override;

    void init() override;
    Q_INVOKABLE void openSystemMonitor();

    KSysGuard::SensorFaceController *faceController() const;

    // Workaround for Bug 424458, when reusing the controller/item things break in ConfigAppearance
    Q_INVOKABLE KSysGuard::SensorFaceController *workaroundController(QQuickItem *context) const;

public Q_SLOTS:
    void configChanged() override;

private:
    KSysGuard::SensorFaceController *m_sensorFaceController = nullptr;
    QString m_pendingStartupPreset;
};
