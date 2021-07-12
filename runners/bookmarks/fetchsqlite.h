/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include <QList>
#include <QSqlDatabase>
#include <QVariantMap>

#include <QString>
#include <QVariant>

#include <QMutex>
#include <QObject>

class FetchSqlite : public QObject
{
    Q_OBJECT
public:
    explicit FetchSqlite(const QString &databaseFile, QObject *parent = nullptr);
    ~FetchSqlite() override;
    void prepare();
    void teardown();
    QList<QVariantMap> query(const QString &sql, QMap<QString, QVariant> bindObjects);
    QList<QVariantMap> query(const QString &sql);
    QStringList tables(QSql::TableType type = QSql::Tables);

private:
    QString const m_databaseFile;
    QMutex m_mutex;
};
