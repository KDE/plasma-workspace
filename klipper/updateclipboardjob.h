/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCompositeJob>

class QMimeData;
class QSqlDatabase;

inline constexpr QLatin1String s_imageFormat("image/png");
inline constexpr QLatin1String s_plainTextPrefix("text/plain");
inline constexpr QLatin1String s_plainUtf8Text("text/plain;charset=utf-8");

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
                                              const QMimeData *mimeData);
    ~UpdateDatabaseJob() override;

    void start() override;

protected:
    explicit UpdateDatabaseJob(QObject *parent,
                               QSqlDatabase *database,
                               QStringView databaseFolder,
                               const QString &uuid,
                               const QString &text,
                               std::list<MimeData> &&mimeDataList);

private:
    void onDataDirReady(KJob *job);
    void slotResult(KJob *job) override;

    QSqlDatabase *m_db = nullptr;
    QString m_uuid;
    QString m_text;
    QString m_dataDir;
    std::list<MimeData> m_mimeDataList;
};
