/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fetchsqlite.h"
#include "bookmarks_debug.h"
#include "bookmarksrunner_defs.h"
#include <QDebug>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <sstream>

FetchSqlite::FetchSqlite(const QString &databaseFile, QObject *parent)
    : QObject(parent)
    , m_databaseFile(databaseFile)
{
}

FetchSqlite::~FetchSqlite()
{
}

void FetchSqlite::prepare()
{
}

void FetchSqlite::teardown()
{
    const QString connectionPrefix = m_databaseFile + "-";
    const auto connections = QSqlDatabase::connectionNames();
    for (const auto &c : connections) {
        if (c.startsWith(connectionPrefix)) {
            qCDebug(RUNNER_BOOKMARKS) << "Closing connection" << c;
            QSqlDatabase::removeDatabase(c);
        }
    }
}

static QSqlDatabase openDbConnection(const QString &databaseFile)
{
    // Try to reuse the previous connection
    auto db = QSqlDatabase::database(databaseFile);
    if (db.isValid()) {
        qCDebug(RUNNER_BOOKMARKS) << "Reusing connection" << databaseFile;
        return db;
    }

    // Otherwise, create, configure and open a new one
    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), databaseFile);
    db.setHostName(QStringLiteral("localhost"));
    db.setDatabaseName(databaseFile);
    db.open();
    qCDebug(RUNNER_BOOKMARKS) << "Opened connection" << databaseFile;

    return db;
}

QList<QVariantMap> FetchSqlite::query(const QString &sql, QMap<QString, QVariant> bindObjects)
{
    auto db = openDbConnection(m_databaseFile);
    if (!db.isValid()) {
        return QList<QVariantMap>();
    }

    // qDebug() << "query: " << sql;
    QSqlQuery query(db);
    query.prepare(sql);
    for (auto entry = bindObjects.constKeyValueBegin(); entry != bindObjects.constKeyValueEnd(); ++entry) {
        query.bindValue((*entry).first, (*entry).second);
        // qDebug() << "* Bound " << variableName << " to " << query.boundValue(variableName);
    }

    if (!query.exec()) {
        QSqlError error = db.lastError();
        // qDebug() << "query failed: " << error.text() << " (" << error.type() << ", " << error.number() << ")";
        // qDebug() << query.lastQuery();
    }

    QList<QVariantMap> result;
    while (query.next()) {
        QVariantMap recordValues;
        QSqlRecord record = query.record();
        for (int field = 0; field < record.count(); field++) {
            QVariant value = record.value(field);
            recordValues.insert(record.fieldName(field), value);
        }
        result << recordValues;
    }

    return result;
}

QStringList FetchSqlite::tables(QSql::TableType type)
{
    auto db = openDbConnection(m_databaseFile);
    return db.tables(type);
}
