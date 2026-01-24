/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QString>

#include <qqmlregistration.h>

/**
 * This class provides a DBus interface for individual Waydroid applications.
 * Each application gets its own DBus object registered at a unique path.
 *
 * @author Florian RICHER <florian.richer@protonmail.com>
 */
class WaydroidApplicationDBusObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasmashell.WaydroidApplication")

public:
    typedef std::shared_ptr<WaydroidApplicationDBusObject> Ptr;

    explicit WaydroidApplicationDBusObject(QObject *parent = nullptr);

    void registerObject();
    void unregisterObject();
    [[nodiscard]] QDBusObjectPath objectPath() const;

    /**
     * @brief Read one application from "waydroid app list" command output log through QTextStream.
     * The QTextStream cursor must be set to the first line of the application.
     * The first line begin with "Name:".
     *
     * @param inFile The QTextStream used to read line by line the Waydroid logs.
     * @return One parsed application DBus object, or std::nullopt if parsing failed
     */
    static Ptr parseApplicationFromWaydroidLog(QTextStream &inFile);

    /**
     * @brief Read all applications from "waydroid app list" command output log through QTextStream.
     *
     * @param inFile The QTextStream used to read line by line the Waydroid logs.
     * @return All parsed application DBus objects
     */
    static QList<Ptr> parseApplicationsFromWaydroidLog(QTextStream &inFile);

public Q_SLOTS:
    Q_SCRIPTABLE QString name() const;
    Q_SCRIPTABLE QString packageName() const;

private:
    bool m_dbusInitialized{false};
    QDBusObjectPath m_objectPath;
    QString m_name;
    QString m_packageName;
};