/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mimedatabase.h"

#include <QBuffer>
#include <QDir>
#include <QSaveFile>
#include <QUrl>

#include <KIO/DeleteJob>
#include <KIO/FileJob>
#include <KIO/StoredTransferJob>

#include "databasemigration.h"
#include "klipper_debug.h"
#include "updateclipboardjob.h"

ReadJob::ReadJob(QObject *parent, const QString &dataPath, const std::optional<QByteArray> &data)
    : KJob(parent)
    , m_dataPath(dataPath)
    , m_data(data)
{
}

ReadJob::~ReadJob()
{
}

void ReadJob::start()
{
    if (m_data.has_value()) {
        QMetaObject::invokeMethod(
            this,
            [this] {
                Q_EMIT data(m_data.value());
                emitResult();
            },
            Qt::QueuedConnection);
    } else {
        auto job = KIO::open(QUrl::fromLocalFile(m_dataPath), QIODevice::ReadOnly);
        connect(job, &KIO::FileJob::open, this, [this, job] {
            connect(job, &KIO::FileJob::data, this, [this, job](KJob *, const QByteArray &data) {
                Q_EMIT this->data(data);
                job->close();
            });
            job->read(job->size());
        });
        connect(job, &KJob::finished, this, [this] {
            emitResult();
        });
    }
}

std::shared_ptr<MimeDatabase> MimeDatabase::self()
{
    static std::weak_ptr<MimeDatabase> s_db;
    if (s_db.expired()) {
        std::shared_ptr<MimeDatabase> ptr{new MimeDatabase};
        s_db = ptr;
        return ptr;
    }
    return s_db.lock();
}

MimeDatabase::MimeDatabase()
    : m_dataFolder(DatabaseMigration::databaseFolder() + +u"/data/")
{
}

MimeDatabase::~MimeDatabase()
{
    if (m_inMemory) {
        QDir(m_dataFolder).removeRecursively();
    }
}

void MimeDatabase::init(bool inMemory, QObject *parent)
{
    m_parent = parent;
    m_inMemory = inMemory;

    QDir dataDir(m_dataFolder);
    if (m_inMemory && dataDir.exists()) {
        dataDir.removeRecursively();
    }
    dataDir.mkpath(dataDir.absolutePath());

    m_inited = true;
}

void MimeDatabase::clear(const QStringList &uuids, int &pendingJobCount)
{
    Q_ASSERT(m_inited);
    if (m_inMemory) {
        for (const QString &uuid : uuids) {
            [[maybe_unused]] const std::size_t count = m_memoryDatabase.erase(uuid);
            Q_ASSERT_X(count == 1UL, Q_FUNC_INFO, qPrintable(uuid));
        }
    }

    // Delete local image data when database is in memory
    QList<QUrl> deletedDataFolders;
    deletedDataFolders.reserve(uuids.size());
    std::transform(uuids.cbegin(), uuids.cend(), std::back_inserter(deletedDataFolders), [this](const QString &uuid) {
        return QUrl::fromLocalFile(m_dataFolder + uuid + u'/');
    });
    auto job = KIO::del(deletedDataFolders, KIO::HideProgressInfo);
    ++pendingJobCount;
    m_parent->connect(job, &KJob::finished, m_parent, [pendingJobCount]() mutable {
        --pendingJobCount;
    });
}

ReadJob *MimeDatabase::asyncRead(const QString &uuid, const QString &dataUuid)
{
    Q_ASSERT(m_inited);
    if (m_inMemory) {
        auto it = m_memoryDatabase.find(uuid);
        if (it != m_memoryDatabase.end()) {
            auto dataIt = it->second.find(dataUuid);
            if (dataIt != it->second.end()) {
                return new ReadJob(m_parent, QString(), dataIt->second);
            }
        }
    }
    return new ReadJob(m_parent, m_dataFolder + uuid + u'/' + dataUuid, std::nullopt);
}

void MimeDatabase::write(const QString &uuid, const QString &dataUuid, const QByteArray &data)
{
    Q_ASSERT(m_inited);
    if (m_inMemory) {
        auto it = m_memoryDatabase.find(uuid);
        if (it == m_memoryDatabase.end()) {
            m_memoryDatabase.emplace(uuid, MimeDataMap{{dataUuid, data}});
        } else {
            it->second.emplace(dataUuid, data);
        }
    } else {
        const QString folderPath = m_dataFolder + uuid;
        QDir().mkpath(folderPath);
        QSaveFile file(folderPath + u'/' + dataUuid);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(KLIPPER_LOG) << file.errorString() << folderPath;
            return;
        }
        file.write(data);
        file.commit();
    }
}

KJob *MimeDatabase::asyncWrite(const QString &uuid, const MimeData &data)
{
    Q_ASSERT(m_inited);
    if (m_inMemory) {
        if (data.type != s_imageFormat) {
            write(uuid, data.uuid, data.data);
            return nullptr;
        }
    }
    return KIO::storedPut(data.data, QUrl::fromLocalFile(QString(m_dataFolder + uuid + u'/' + data.uuid)), -1, KIO::HideProgressInfo);
}

bool MimeDatabase::inMemory() const
{
    return m_inMemory;
}
