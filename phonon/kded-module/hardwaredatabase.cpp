/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 or version 3 as published by the Free Software
    Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "hardwaredatabase.h"
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <QtCore/QCache>
#include <QtCore/QList>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVector>
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QStandardPaths>

static const char CACHE_MAGIC[] = "PHwdbC";
static const quint32 CACHE_VERSION = 2;
static const quint32 HEADER_LENGTH = 14;

namespace PS
{
namespace HardwareDatabase
{

class HardwareDatabasePrivate : public QObject
{
    public:
        HardwareDatabasePrivate();
        void createCache(const QString &dbFileName, const QString &cacheFileName);
        bool validCacheHeader(QDataStream &cacheStream);
        Entry *readEntry(const QString &uniqueId);

        QCache<QString, Entry> entryCache;

    private:
        QWeakPointer<QFile> m_cacheFile;
        QString m_fileName;
};

K_GLOBAL_STATIC(HardwareDatabasePrivate, s_globalDB)

HardwareDatabasePrivate::HardwareDatabasePrivate()
    : m_cacheFile()
{
    const QString dbFileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("libphonon/hardwaredatabase"));
    if (dbFileName.isEmpty()) {
        // no database, we're useless
        return;
    }
    const QString cacheFileName =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/libphonon/hardwaredatabase/");
    const QFileInfo dbFileInfo(dbFileName);
    const QFileInfo cacheFileInfo(cacheFileName);
    if (!cacheFileInfo.exists() || cacheFileInfo.lastModified() < dbFileInfo.lastModified()) {
        // update cache file
        createCache(dbFileName, cacheFileName);
    } else {
        m_cacheFile = new QFile(cacheFileName);
        m_cacheFile.data()->open(QIODevice::ReadOnly);
        m_cacheFile.data()->deleteLater();
        QDataStream cacheStream(m_cacheFile.data());
        if (!validCacheHeader(cacheStream)) {
            m_cacheFile.data()->close();
            delete m_cacheFile.data();
            createCache(dbFileName, cacheFileName);
        }
    }
    m_fileName = cacheFileName;
}

struct BucketEntry
{
    BucketEntry() {}
    BucketEntry(const uint &a, const quint32 &b) : hash(a), offset(b) {}
    uint hash;
    quint32 offset;
};

QDataStream &operator<<(QDataStream &s, const BucketEntry &e)
{
    return s << e.hash << e.offset;
}

QDataStream &operator>>(QDataStream &s, BucketEntry &e)
{
    return s >> e.hash >> e.offset;
}

void HardwareDatabasePrivate::createCache(const QString &dbFileName, const QString &cacheFileName)
{
    KSaveFile cacheFile(cacheFileName);
    QString name;
    QString iconName;
    int pref;
    quint8 isAdvanced;

    const KConfig dbFile(dbFileName, KConfig::CascadeConfig);
    const bool opened = cacheFile.open(); // QIODevice::WriteOnly
    Q_ASSERT(opened); Q_UNUSED(opened);
    QDataStream cacheStream(&cacheFile);
    cacheStream.writeRawData(CACHE_MAGIC, 6);
    cacheStream << CACHE_VERSION << cacheStream.version() << quint32(0) << quint32(0);
    QStringList allIds = dbFile.groupList();
    QHash<uint, quint32> offsets;
    offsets.reserve(allIds.count());
    foreach (const QString &uniqueId, allIds) {
        offsets.insertMulti(qHash(uniqueId), cacheFile.pos());
        const KConfigGroup group = dbFile.group(uniqueId);
        name = group.readEntry("name", QString());
        iconName = group.readEntry("icon", QString());
        pref = group.readEntry("initialPreference", 0);
        if (group.hasKey("isAdvancedDevice")) {
            isAdvanced = group.readEntry("isAdvancedDevice", false) ? 1 : 0;
        } else {
            isAdvanced = 2;
        }
        cacheStream << uniqueId << name << iconName << pref << isAdvanced;
    }
    //offsets.squeeze();
    const quint32 hashTableBuckets = offsets.capacity();
    const quint32 hashTableOffset = cacheFile.pos();
    QVector<QList<BucketEntry> > bucketContents(hashTableBuckets);
    {
        QHashIterator<uint, quint32> it(offsets);
        while (it.hasNext()) {
            it.next();
            const uint &h = it.key();
            bucketContents[h % hashTableBuckets] << BucketEntry(h, it.value());
        }
        offsets.clear();
    }
    for (quint32 i = 0; i < hashTableBuckets; ++i) {
        cacheStream << quint32(0);
    }
    QVarLengthArray<quint32, 4099> bucketOffsets(hashTableBuckets);
    for (quint32 i = 0; i < hashTableBuckets; ++i) {
        if (bucketContents[i].isEmpty()) {
            bucketOffsets[i] = 0;
        } else {
            bucketOffsets[i] = cacheFile.pos();
            cacheStream << bucketContents[i];
        }
    }
    bucketContents.clear();
    cacheFile.seek(hashTableOffset);
    for (quint32 i = 0; i < hashTableBuckets; ++i) {
        cacheStream << bucketOffsets[i];
    }
    cacheFile.seek(HEADER_LENGTH);
    cacheStream << hashTableOffset << hashTableBuckets;
    cacheFile.close();
}

bool HardwareDatabasePrivate::validCacheHeader(QDataStream &cacheStream)
{
    char magic[6];
    quint32 version;
    int datastreamVersion;
    const int read = cacheStream.readRawData(magic, 6);
    cacheStream >> version >> datastreamVersion;
    return (read == 6 && 0 == strncmp(magic, CACHE_MAGIC, 6) && version == CACHE_VERSION && datastreamVersion == cacheStream.version());
}

Entry *HardwareDatabasePrivate::readEntry(const QString &uniqueId)
{
    QDataStream cacheStream;
    if (m_cacheFile) {
        if (m_cacheFile.data()->seek(HEADER_LENGTH)) {
            cacheStream.setDevice(m_cacheFile.data());
        } else {
            delete m_cacheFile.data();
        }
    }
    if (!m_cacheFile) {
        m_cacheFile = new QFile(m_fileName);
        m_cacheFile.data()->open(QIODevice::ReadOnly);
        m_cacheFile.data()->deleteLater();
        cacheStream.setDevice(m_cacheFile.data());
        if (!validCacheHeader(cacheStream)) {
            return 0;
        }
    }
    quint32 hashTableOffset;
    quint32 hashTableBuckets;
    cacheStream >> hashTableOffset >> hashTableBuckets;
    const uint h = qHash(uniqueId);
    m_cacheFile.data()->seek(hashTableOffset + (h % hashTableBuckets) * sizeof(quint32));
    quint32 offset;
    cacheStream >> offset;
    //kDebug(601) << hashTableOffset << hashTableBuckets << uniqueId << h << offset;
    if (0 == offset) {
        return 0;
    }
    m_cacheFile.data()->seek(offset);
    QList<BucketEntry> bucket;
    cacheStream >> bucket;

    QString readUdi;
    QString name;
    QString iconName;
    int pref;
    quint8 isAdvanced;

    foreach (const BucketEntry &entry, bucket) {
        if (entry.hash == h) {
            m_cacheFile.data()->seek(entry.offset);
            cacheStream >> readUdi;
            if (readUdi == uniqueId) {
                cacheStream >> name >> iconName >> pref >> isAdvanced;
                Entry *e = new Entry(name, iconName, pref, isAdvanced);
                s_globalDB->entryCache.insert(uniqueId, e);
                return e;
            }
        }
    }

    return 0;
}

bool contains(const QString &uniqueId)
{
    return (s_globalDB->entryCache[uniqueId] || s_globalDB->readEntry(uniqueId));
}

Entry entryFor(const QString &uniqueId)
{
    Entry *e = s_globalDB->entryCache[uniqueId];
    if (e) {
        return *e;
    }
    e = s_globalDB->readEntry(uniqueId);
    if (e) {
        return *e;
    }
    return Entry();
}

} // namespace HardwareDatabase
} // namespace PS
