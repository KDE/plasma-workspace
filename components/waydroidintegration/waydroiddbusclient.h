/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "plasmashellwaydroidinterface.h"
#include "waydroidapplicationlistmodel.h"
#include "waydroiddbusobject.h"

#include <QCoroCore>
#include <QCoroQmlTask>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QString>

#include <qqmlregistration.h>

class WaydroidDBusClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(SessionStatus sessionStatus READ sessionStatus NOTIFY sessionStatusChanged)
    Q_PROPERTY(SystemType systemType READ systemType NOTIFY systemTypeChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(QString androidId READ androidId NOTIFY androidIdChanged)
    Q_PROPERTY(bool multiWindows READ multiWindows WRITE setMultiWindows NOTIFY multiWindowsChanged)
    Q_PROPERTY(bool suspend READ suspend WRITE setSuspend NOTIFY suspendChanged)
    Q_PROPERTY(bool uevent READ uevent WRITE setUevent NOTIFY ueventChanged)
    Q_PROPERTY(WaydroidApplicationListModel *applicationListModel READ applicationListModel CONSTANT)

public:
    explicit WaydroidDBusClient(QObject *parent = nullptr);

    /**
     * @enum Status
     * @brief Defines the possible installation statuses of the Waydroid service.
     */
    enum Status {
        NotSupported = WaydroidDBusObject::NotSupported,
        NotInitialized = WaydroidDBusObject::NotInitialized,
        Initializing = WaydroidDBusObject::Initializing,
        Initialized = WaydroidDBusObject::Initialized,
        Resetting = WaydroidDBusObject::Resetting,
    };
    Q_ENUM(Status)

    /**
     * @enum SessionStatus
     * @brief Defines the possible states of a Waydroid session.
     */
    enum SessionStatus {
        SessionStopped = WaydroidDBusObject::SessionStopped,
        SessionStarting = WaydroidDBusObject::SessionStarting,
        SessionRunning = WaydroidDBusObject::SessionRunning,
    };
    Q_ENUM(SessionStatus)

    /**
     * @enum SystemType
     * @brief Defines the types of Android systems supported by Waydroid.
     */
    enum SystemType {
        Vanilla = WaydroidDBusObject::Vanilla, ///< Vanilla Android system.
        Foss = WaydroidDBusObject::Foss, ///< Free and Open Source Software variant.
        Gapps = WaydroidDBusObject::Gapps, ///< Variant with Google Apps included.
        UnknownSystemType = WaydroidDBusObject::UnknownSystemType
    };
    Q_ENUM(SystemType)

    /**
     * @enum RomType
     * @brief Defines the types of ROMs supported by Waydroid.
     *
     * @todo Add OTA ROM with custom system url and vendor url
     */
    enum RomType {
        Lineage = WaydroidDBusObject::Lineage, ///< LineageOS ROM.
        Bliss = WaydroidDBusObject::Bliss ///< Bliss ROM.
    };
    Q_ENUM(RomType)

    [[nodiscard]] Status status() const;
    [[nodiscard]] SessionStatus sessionStatus() const;
    [[nodiscard]] SystemType systemType() const;
    [[nodiscard]] QString ipAddress() const;
    [[nodiscard]] QString androidId() const;
    [[nodiscard]] WaydroidApplicationListModel *applicationListModel() const;

    [[nodiscard]] bool multiWindows() const;
    QCoro::QmlTask setMultiWindows(const bool multiWindows);
    [[nodiscard]] bool suspend() const;
    QCoro::QmlTask setSuspend(const bool suspend);
    [[nodiscard]] bool uevent() const;
    QCoro::QmlTask setUevent(const bool uevent);

    Q_INVOKABLE QCoro::QmlTask initialize(const SystemType systemType, const RomType romType, const bool forced = false);
    Q_INVOKABLE QCoro::QmlTask startSession();
    Q_INVOKABLE QCoro::QmlTask stopSession();
    Q_INVOKABLE QCoro::QmlTask resetWaydroid();
    Q_INVOKABLE QCoro::QmlTask installApk(const QString apkFile);
    Q_INVOKABLE QCoro::QmlTask deleteApplication(const QString appId);
    Q_INVOKABLE QCoro::QmlTask refreshSessionInfo();
    Q_INVOKABLE QCoro::QmlTask refreshAndroidId();
    Q_INVOKABLE QCoro::QmlTask refreshApplications();

    Q_INVOKABLE void copyToClipboard(const QString text);

Q_SIGNALS:
    void statusChanged();
    // download and total is in MB and speed in Kbps
    void downloadStatusChanged(double downloaded, double total, double speed);
    void sessionStatusChanged();
    void systemTypeChanged();
    void ipAddressChanged();
    void androidIdChanged();
    void multiWindowsChanged();
    void suspendChanged();
    void ueventChanged();

    void actionFinished(const QString message);
    void actionFailed(const QString message);
    void errorOccurred(const QString title, const QString message);

private Q_SLOTS:
    void updateStatus();
    void updateSessionStatus();
    void updateSystemType();
    void updateIpAddress();
    void updateAndroidId();
    void updateMultiWindows();
    void updateSuspend();
    void updateUevent();

private:
    OrgKdePlasmashellWaydroidInterface *m_interface;
    QDBusServiceWatcher *m_watcher;

    Status m_status{NotInitialized};
    SessionStatus m_sessionStatus{SessionStopped};
    SystemType m_systemType{UnknownSystemType};
    QString m_ipAddress;
    QString m_androidId;
    WaydroidApplicationListModel *m_applicationListModel{nullptr};

    // Waydroid props. See https://docs.waydro.id/usage/waydroid-prop-options
    bool m_multiWindows{false};
    bool m_suspend{false};
    bool m_uevent{false};

    bool m_connected{false};

    void connectSignals();
    void initializeApplicationListModel();

    QCoro::Task<void> initializeTask(const SystemType systemType, const RomType romType, const bool forced = false);
    QCoro::Task<void> startSessionTask();
    QCoro::Task<void> stopSessionTask();
    QCoro::Task<void> resetWaydroidTask();
    QCoro::Task<void> installApkTask(const QString apkFile);
    QCoro::Task<void> deleteApplicationTask(const QString appId);
    QCoro::Task<void> setMultiWindowsTask(const bool multiWindows);
    QCoro::Task<void> setSuspendTask(const bool suspend);
    QCoro::Task<void> setUeventTask(const bool uevent);
    QCoro::Task<void> refreshSessionInfoTask();
    QCoro::Task<void> refreshAndroidIdTask();
    QCoro::Task<void> refreshApplicationsTask();
};
