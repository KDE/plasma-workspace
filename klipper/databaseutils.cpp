/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "databaseutils.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include "klipper_debug.h"

using namespace Qt::StringLiterals;

namespace DatabaseUtils
{
QString databaseFolder()
{
    if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
        return QFileInfo(qEnvironmentVariable("KLIPPER_DATABASE")).absoluteDir().absolutePath();
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper";
}

QString databaseName(Location location)
{
    if (location == Memory) {
        return u"file::memory:"_s;
    }
    // Don't use "appdata", klipper is also a kicker applet
    if (qEnvironmentVariableIsSet("KLIPPER_DATABASE")) {
        return qEnvironmentVariable("KLIPPER_DATABASE");
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/history3.sqlite";
}

QString databaseConnectionName(Location location)
{
    return location == Memory ? u"klipper_mem"_s : u"klipper"_s;
}

std::optional<QSqlDatabase> openDatabase(Location location)
{
    const QString name = databaseConnectionName(location);
    // Try to reuse the previous connection
    QSqlDatabase db = QSqlDatabase::database(name, true);
    if (db.isOpen()) {
        return db;
    }

    db = QSqlDatabase::addDatabase(u"QSQLITE"_s, name);
    db.setHostName(u"localhost"_s);
    if (location == Memory) {
        db.setConnectOptions(u"QSQLITE_OPEN_URI"_s);
    }
    db.setDatabaseName(databaseName(location));

    Q_ASSERT(!db.isOpen());
    if (!db.open()) {
        qCWarning(KLIPPER_LOG) << "Failed to open database:" << db.lastError().text();
        return std::nullopt;
    }

    // Initialize the database
    QSqlQuery query(db);
    if (location == LocalFile) {
        query.exec(u"PRAGMA journal_mode=WAL"_s);
    }

    // The main table only stores text data
    query.exec(
        u"CREATE TABLE IF NOT EXISTS main (uuid char(40) PRIMARY KEY, added_time REAL NOT NULL CHECK (added_time > 0), last_used_time REAL CHECK (last_used_time > 0), mimetypes TEXT NOT NULL, text NTEXT, starred BOOLEAN)"_s);
    // The aux table stores data index
    query.exec(u"CREATE TABLE IF NOT EXISTS aux (uuid char(40) NOT NULL, mimetype TEXT NOT NULL, data_uuid char(40) NOT NULL, PRIMARY KEY (uuid, mimetype))"_s);
    // Save the latest version number
    query.exec(u"CREATE TABLE IF NOT EXISTS version (db_version INT NOT NULL)"_s);
    constexpr int currentDBVersion = 3;
    if (query.exec(u"SELECT db_version FROM version"_s) && query.isSelect() && query.next() /* has a record */) {
        if (query.value(0).toInt() != currentDBVersion) {
            return std::nullopt;
        }
    } else if (!query.exec(u"INSERT INTO version (db_version) VALUES (%1)"_s.arg(QString::number(currentDBVersion)))) {
        qCWarning(KLIPPER_LOG) << "Failed to write to database:" << db.lastError().text();
        return std::nullopt;
    }

    return db;
}

std::optional<QSqlDatabase> migrate(Direction direction)
{
    std::optional<QSqlDatabase> sourcedb = openDatabase(direction == MemToFile ? Memory : LocalFile);
    if (!sourcedb.has_value()) {
        return std::nullopt;
    }

    QSqlQuery query(sourcedb.value());
    bool ret = true;
    if (direction == MemToFile) {
        ret = query.exec(u"ATTACH DATABASE '%1' AS targetdb"_s.arg(databaseName(LocalFile)));
        if (!ret) {
            qCWarning(KLIPPER_LOG) << "Failed to attach the local database:" << query.lastError();
            Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(query.lastError().text()));
            return std::nullopt;
        }

        for (auto table : {u"main", u"aux", u"version"}) {
            query.exec(u"DROP TABLE IF EXISTS targetdb.%1"_s.arg(table));
            ret = query.exec(u"CREATE TABLE targetdb.%1 AS SELECT * FROM %1"_s.arg(table));
            if (!ret) {
                qCWarning(KLIPPER_LOG) << "Failed to create table:" << query.lastError();
                Q_ASSERT_X(false, Q_FUNC_INFO, qUtf8Printable(query.lastError().text()));
                return std::nullopt;
            }
        }
        return openDatabase(LocalFile);
    }

    return std::nullopt;
}
}
