/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#ifndef FETCHSQLITE_H
#define FETCHSQLITE_H
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

#endif // FETCHSQLITE_H
