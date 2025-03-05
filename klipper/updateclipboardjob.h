/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCompositeJob>

class QMimeData;
class QSqlDatabase;
class MimeDatabase;

inline constexpr QLatin1String s_imageFormat("image/png");
inline constexpr QLatin1String s_plainTextPrefix("text/plain;charset=");

struct MimeData {
    QString type;
    QByteArray data;
    QString uuid;
};

/**
 * A composite job that saves a clip to a local folder and updates the database
 */
class UpdateDatabaseJob : public KCompositeJob
{
    Q_OBJECT

public:
    enum {
        DatabaseError = KJob::UserDefinedError,
    };

    static UpdateDatabaseJob *updateClipboard(QObject *parent,
                                              QSqlDatabase *database,
                                              QStringView databaseFolder,
                                              const QString &uuid,
                                              const QString &text,
                                              const QMimeData *mimeData,
                                              qreal timestamp);
    ~UpdateDatabaseJob() override;

    void start() override;

protected:
    explicit UpdateDatabaseJob(QObject *parent,
                               QSqlDatabase *database,
                               QStringView databaseFolder,
                               const QString &uuid,
                               const QString &text,
                               const QStringList &formats,
                               std::list<MimeData> &&mimeDataList,
                               qreal timestamp);

private:
    void onDataDirReady(KJob *job);
    void slotResult(KJob *job) override;

    QSqlDatabase *m_db = nullptr;
    std::shared_ptr<MimeDatabase> m_mimedb;
    QString m_uuid;
    QString m_text;
    QStringList m_formats;
    QString m_dataDir;
    std::list<MimeData> m_mimeDataList;
    qreal m_timestamp;
};
