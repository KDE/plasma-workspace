/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QIODevice>

#include <KJob>

class MimeData;
class UpdateDatabaseJob;

class ReadJob : public KJob
{
    Q_OBJECT

public:
    explicit ReadJob(QObject *parent, const QString &dataPath, const std::optional<QByteArray> &data);
    ~ReadJob() override;

    void start() override;

Q_SIGNALS:
    void data(const QByteArray &data);

private:
    QString m_dataPath;
    std::optional<QByteArray> m_data;
};

class MimeDatabase
{
public:
    static std::shared_ptr<MimeDatabase> self();
    ~MimeDatabase();

    void init(bool inMemory, QObject *parent);
    void clear(const QStringList &uuids, int &pendingJobCount);

    ReadJob *asyncRead(const QString &uuid, const QString &dataUuid);
    void write(const QString &uuid, const QString &dataUuid, const QByteArray &data);
    KJob *asyncWrite(const QString &uuid, const MimeData &data);

    bool inMemory() const;

private:
    explicit MimeDatabase();
    bool m_inited = false;
    QObject *m_parent;
    QString m_dataFolder;
    bool m_inMemory = false;

    using MimeDataMap = std::unordered_map<QString /*data_uuid*/, QByteArray /*data*/>;
    std::unordered_map<QString /*uuid*/, MimeDataMap> m_memoryDatabase;
};
