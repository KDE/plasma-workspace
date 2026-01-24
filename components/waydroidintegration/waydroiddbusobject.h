/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "waydroidapplicationdbusobject.h"

#include <QCoroTask>
#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QString>

#include <qqmlregistration.h>

class WaydroidApplicationDBusObject;

/**
 * This class provides an interface to interact with the Waydroid container,
 * including session management and property configuration.
 *
 * @author Florian RICHER <florian.richer@protonmail.com>
 */
class WaydroidDBusObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasmashell.Waydroid")

public:
    explicit WaydroidDBusObject(QObject *parent = nullptr);

    /**
     * @enum Status
     * @brief Defines the possible installation statuses of the Waydroid service.
     */
    enum Status {
        NotSupported = 0,
        NotInitialized,
        Initializing,
        Initialized,
        Resetting
    };
    Q_ENUM(Status)

    /**
     * @enum SessionStatus
     * @brief Defines the possible states of a Waydroid session.
     */
    enum SessionStatus {
        SessionStopped = 0,
        SessionStarting,
        SessionRunning
    };
    Q_ENUM(SessionStatus)

    /**
     * @enum SystemType
     * @brief Defines the types of Android systems supported by Waydroid.
     */
    enum SystemType {
        Vanilla = 0, ///< Vanilla Android system.
        Foss, ///< Free and Open Source Software variant.
        Gapps, ///< Variant with Google Apps included.
        UnknownSystemType
    };
    Q_ENUM(SystemType)

    /**
     * @enum RomType
     * @brief Defines the types of ROMs supported by Waydroid.
     *
     * @todo Add OTA ROM with custom system url and vendor url
     */
    enum RomType {
        Lineage = 0, ///< LineageOS ROM.
        Bliss ///< Bliss ROM.
    };
    Q_ENUM(RomType)

    // called by QML
    Q_INVOKABLE void registerObject();

Q_SIGNALS:
    Q_SCRIPTABLE void statusChanged();
    // download and total is in MB and speed in Kbps
    Q_SCRIPTABLE void downloadStatusChanged(double downloaded, double total, double speed);
    Q_SCRIPTABLE void sessionStatusChanged();
    Q_SCRIPTABLE void systemTypeChanged();
    Q_SCRIPTABLE void ipAddressChanged();
    Q_SCRIPTABLE void androidIdChanged();
    Q_SCRIPTABLE void multiWindowsChanged();
    Q_SCRIPTABLE void suspendChanged();
    Q_SCRIPTABLE void ueventChanged();

    Q_SCRIPTABLE void applicationAdded(QDBusObjectPath path);
    Q_SCRIPTABLE void applicationRemoved(QDBusObjectPath path);

    // Use to display banner
    Q_SCRIPTABLE void actionFinished(const QString message);
    Q_SCRIPTABLE void actionFailed(const QString message);

    // General error
    Q_SCRIPTABLE void errorOccurred(const QString title, const QString message);

public Q_SLOTS:
    Q_SCRIPTABLE int status() const;
    Q_SCRIPTABLE int sessionStatus() const;
    Q_SCRIPTABLE int systemType() const;
    Q_SCRIPTABLE QString ipAddress() const;
    Q_SCRIPTABLE QString androidId() const;
    Q_SCRIPTABLE bool multiWindows() const;
    Q_SCRIPTABLE void setMultiWindows(const bool multiWindows);
    Q_SCRIPTABLE bool suspend() const;
    Q_SCRIPTABLE void setSuspend(const bool suspend);
    Q_SCRIPTABLE bool uevent() const;
    Q_SCRIPTABLE void setUevent(const bool uevent);
    Q_SCRIPTABLE QList<QDBusObjectPath> applications() const;

    Q_SCRIPTABLE void initialize(const int systemType, const int romType, const bool forced = false);
    Q_SCRIPTABLE void startSession();
    Q_SCRIPTABLE void stopSession();
    Q_SCRIPTABLE void resetWaydroid();
    Q_SCRIPTABLE void installApk(const QString apkFile);
    Q_SCRIPTABLE void deleteApplication(const QString appId);
    Q_SCRIPTABLE void refreshSessionInfo();
    Q_SCRIPTABLE void refreshAndroidId();
    Q_SCRIPTABLE void refreshApplications();

private:
    bool m_dbusInitialized{false};
    Status m_status{NotInitialized};
    SessionStatus m_sessionStatus{SessionStopped};
    SystemType m_systemType{UnknownSystemType};
    QString m_ipAddress;
    QString m_androidId;

    // Waydroid props. See https://docs.waydro.id/usage/waydroid-prop-options
    bool m_multiWindows{false};
    bool m_suspend{false};
    bool m_uevent{false};

    void refreshSupportsInfo();
    void refreshInstallationInfo();
    QCoro::Task<void> refreshPropsInfo();

    /**
     * @brief Executes the command to retrieve the current session status and related
     * information from Waydroid.
     *
     * @return A QString containing the output of the Waydroid session status command.
     */
    QCoro::Task<QString> fetchSessionInfo();

    /**
     * @brief Executes the command to retrieve the value of a specified property from the Waydroid container.
     *
     * @param key The key of the property to fetch.
     * @param defaultValue The default value to return if the property is not found or empty.
     * @return A QString containing the property value, or the defaultValue if not found.
     */
    QCoro::Task<QString> fetchPropValue(const QString key, const QString defaultValue);

    /**
     * @brief Executes the command to writes a value to a specified property in the Waydroid container.
     *
     * @param key The key of the property to set.
     * @param value The value to write to the property.
     * @return A boolean indicating whether the write operation was successful.
     */
    QCoro::Task<bool> writePropValue(const QString key, const QString value);

    /**
     * @brief Extracts text from a string using a regular expression pattern.
     *
     * @param text The text to search within.
     * @param regExp The regular expression pattern to use for extraction.
     * @return A QString containing the extracted text if a match is found; otherwise, an empty string.
     */
    QString extractRegExp(const QString text, const QRegularExpression regExp) const;

    /**
     * @brief Checks every 500ms if the session has started.
     *
     * This function periodically checks whether a session has started. If the session starts,
     * it emits a "Running" signal. If the check count reaches the specified limit without
     * the session starting, it emits a "Stopped" signal and logs a warning message.
     *
     * @param limit The maximum number of attempts to check for session start before stopping.
     * @param tried The current number of attempts made to check for session start (defaults to 0).
     *
     * @todo Investigate using DBus for a cleaner implementation, potentially using the method:
     *       id.waydro.Container /ContainerManager id.waydro.ContainerManager.Start(a{ss} session).
     *       This would require duplicating the session start command logic from:
     *       https://github.com/waydroid/waydroid/blob/2c41162d8bfef5bf83333a6ce4834af0c3c2b535/tools/actions/session_manager.py#L31
     */
    void checkSessionStarting(const int limit, const int tried = 0);

    QString desktopFileDirectory();
    bool removeWaydroidApplications();

    QCoro::Task<QString> fetchApplicationsList();
    QList<WaydroidApplicationDBusObject::Ptr> m_applicationObjects;
};
