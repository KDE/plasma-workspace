/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagepackage.h"

#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>

#include "distance.h"
#include "suffixcheck.h"

namespace KPackage
{
int keyToNumberWithBounds(const QJsonObject &timeObject, const QLatin1String &key, int low, int high, int defaultValue)
{
    const auto it = timeObject.constFind(key);
    if (it == timeObject.constEnd()) {
        return defaultValue;
    }
    if (it->isDouble()) {
        return std::clamp(it->toInt(), low, high);
    } else if (it->isString()) {
        return std::clamp(it->toString().toInt(), low, high);
    }
    return defaultValue;
}

QTime parseStartTime(const QJsonObject &timeObject)
{
    // Make sure the time is valid
    int hour = keyToNumberWithBounds(timeObject, QLatin1String("Hour"), 0, 23, 0);
    int minute = keyToNumberWithBounds(timeObject, QLatin1String("Minute"), 0, 59, 0);
    int second = keyToNumberWithBounds(timeObject, QLatin1String("Second"), 0, 59, 0);

    return QTime(hour, minute, second);
}

QDateTime parseStartDateTime(const QJsonObject &wallpaperObject)
{
    const QDateTime startDateTime = QDate::currentDate().startOfDay().addDays(-1);

    const auto it = wallpaperObject.constFind(QLatin1String("StartTime"));
    if (it == wallpaperObject.constEnd()) {
        return startDateTime;
    }

    if (it->isObject()) {
        const QJsonObject timeObject = it->toObject();
        if (timeObject.empty()) {
            return startDateTime;
        }

        int year = keyToNumberWithBounds(timeObject, QLatin1String("Year"), 1, std::numeric_limits<int>::max(), startDateTime.date().year());
        int month = keyToNumberWithBounds(timeObject, QLatin1String("Month"), 1, 12, startDateTime.date().month());
        int day = keyToNumberWithBounds(timeObject, QLatin1String("Day"), 1, 31, startDateTime.date().day());

        QDate startDate(year, month, day);
        if (!startDate.isValid() || startDate > startDateTime.date()) {
            startDate = startDateTime.date();
        }

        return QDateTime(startDate, parseStartTime(timeObject));

    } else if (it->isString()) {
        // Specified in datetime string format
        QDateTime dateTimeFromString = QDateTime::fromString(it->toString(), QStringLiteral("yyyy/MM/dd HH:mm:ss"));
        if (dateTimeFromString.isValid() && dateTimeFromString.date() <= startDateTime.date()) {
            return dateTimeFromString;
        }

        dateTimeFromString = QDateTime::fromString(it->toString(), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        if (dateTimeFromString.isValid() && dateTimeFromString.date() <= startDateTime.date()) {
            return dateTimeFromString;
        }
    }

    return startDateTime;
}

QJsonArray extractMetaData(const QJsonObject &wallpaperObject)
{
    auto it = wallpaperObject.constFind(QLatin1String("Metadata"));
    if (it == wallpaperObject.constEnd() || !it->isArray()) {
        it = wallpaperObject.constFind(QLatin1String("MetaData"));
        if (it == wallpaperObject.constEnd() || !it->isArray()) {
            it = wallpaperObject.constFind(QLatin1String("Meta"));
            if (it == wallpaperObject.constEnd() || !it->isArray()) {
                return {};
            }
        }
    }

    return it->toArray();
}

QString parseFileName(const QJsonObject &itemObject, const QString &packagePath, const QSize &targetSize)
{
    auto fileIt = itemObject.constFind(QLatin1String("FileName"));
    if (fileIt == itemObject.constEnd() || !fileIt->isString()) {
        fileIt = itemObject.constFind(QLatin1String("Filename"));
        if (fileIt == itemObject.constEnd() || !fileIt->isString()) {
            return {};
        }
    }

    QString filename = fileIt->toString();
    // Remove file://
    if (filename.startsWith(QLatin1String("file://"))) {
        filename = filename.mid(7);
    }

    // Convert relative path to absolute path
    QFileInfo info(filename);
    if (info.isRelative()) {
        info.setFile(QDir(packagePath).absoluteFilePath(filename));
    }

    // Convert ${resolution} to widthxheight
    info.setFile(parsePreferredImage(info.absoluteFilePath(), targetSize));

    if (!info.exists() || !isAcceptableSuffix(info.suffix())) {
        return {};
    }

    return info.absoluteFilePath();
}

DynamicMetadataItem::Type parseMetadataItemType(const QJsonObject &itemObject)
{
    const auto typeIt = itemObject.constFind(QLatin1String("Type"));
    if (typeIt != itemObject.constEnd() && typeIt->isString() && typeIt->toString().compare(QLatin1String("transition")) == 0) {
        return DynamicMetadataItem::Transition;
    }
    return DynamicMetadataItem::Static;
}

std::vector<DynamicMetadataItem> parseSolarMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize)
{
    std::vector<DynamicMetadataItem> items;

    const QJsonArray metadata = extractMetaData(wallpaperObject);
    if (metadata.empty()) {
        return items;
    }

    items.reserve(metadata.size());

    for (int i = 0; i < metadata.count(); ++i) {
        const QJsonObject itemObject = metadata.at(i).toObject();
        if (itemObject.isEmpty()) {
            continue; // Skip faulty item
        }

        DynamicMetadataItem item;

        item.filename = parseFileName(itemObject, packagePath, targetSize);
        if (item.filename.isEmpty()) {
            continue;
        }

        const auto azimuthIt = itemObject.constFind(QLatin1String("SolarAzimuth"));
        if (azimuthIt != itemObject.constEnd() && azimuthIt->isDouble()) {
            item.solarAzimuth = std::clamp(azimuthIt->toDouble(), 0.0, 360.0);
        }

        const auto elevationIt = itemObject.constFind(QLatin1String("SolarElevation"));
        if (elevationIt != itemObject.constEnd() && elevationIt->isDouble()) {
            item.solarElevation = std::clamp(elevationIt->toDouble(), -90.0, 90.0);
        }

        const auto startTimeIt = itemObject.constFind(QLatin1String("StartTime"));
        if (startTimeIt != itemObject.constEnd() && startTimeIt->isObject()) {
            item.startTime = parseStartTime(startTimeIt->toObject());
        } else {
            // TODO: generate time based on solar position
        }

        item.type = parseMetadataItemType(itemObject);

        items.emplace_back(item);
    }

    // TODO: sort list based on solar position

    return items;
}

std::vector<DynamicMetadataItem> parseDayNightMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize)
{
    std::vector<DynamicMetadataItem> items;

    const QJsonArray metadata = extractMetaData(wallpaperObject);
    if (metadata.empty()) {
        return items;
    }

    items.reserve(metadata.size());

    for (int i = 0; i < metadata.count(); ++i) {
        const QJsonObject itemObject = metadata.at(i).toObject();
        if (itemObject.isEmpty()) {
            continue; // Skip faulty item
        }

        DynamicMetadataItem item;

        auto todIt = itemObject.constFind(QLatin1String("TimeOfDay"));
        if (todIt != itemObject.constEnd() && todIt->isString()) {
            if (todIt->toString().compare(QLatin1String("night"), Qt::CaseInsensitive)) {
                item.timeOfDay = DynamicMetadataItem::Night;
            }
        }

        item.filename = parseFileName(itemObject, packagePath, targetSize);
        if (item.filename.isEmpty()) {
            continue;
        }

        item.type = parseMetadataItemType(itemObject);

        items.emplace_back(item);
    }

    std::sort(items.begin(), items.end(), [](const DynamicMetadataItem &a, const DynamicMetadataItem &b) {
        return a.timeOfDay < b.timeOfDay;
    });

    return items;
}

std::vector<DynamicMetadataItem> parseTimedMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize)
{
    std::vector<DynamicMetadataItem> items;

    const QJsonArray metadata = extractMetaData(wallpaperObject);
    if (metadata.empty()) {
        return items;
    }

    items.reserve(metadata.size());

    for (int i = 0; i < metadata.count(); ++i) {
        const QJsonObject itemObject = metadata.at(i).toObject();
        if (itemObject.isEmpty()) {
            continue; // Skip faulty item
        }

        DynamicMetadataItem item;

        item.filename = parseFileName(itemObject, packagePath, targetSize);
        if (item.filename.isEmpty()) {
            continue;
        }

        const auto durationIt = itemObject.constFind(QLatin1String("Duration"));
        if (durationIt != itemObject.constEnd() && durationIt->isDouble()) {
            item.duration = std::clamp<quint64>(durationIt->toDouble(), 1, std::numeric_limits<quint64>::max());
        }

        item.type = parseMetadataItemType(itemObject);

        items.emplace_back(item);
    }

    return items;
}

void findPreferredImage(ImagePackage &package, const QSize &targetSize)
{
    auto findBestMatch = [&](const QByteArray &folder) {
        QString preferred;
        const QStringList images = package.entryList(folder);

        if (images.empty()) {
            return preferred;
        }

        float best = std::numeric_limits<float>::max();

        for (const QString &entry : images) {
            QSize candidate = resSize(QFileInfo(entry).baseName());

            if (candidate.isEmpty()) {
                continue;
            }

            const float dist = distance(candidate, targetSize);

            if (preferred.isEmpty() || dist < best) {
                preferred = entry;
                best = dist;
            }
        }

        return preferred;
    };

    const QString preferred = findBestMatch(QByteArrayLiteral("images"));
    const QString preferredDark = findBestMatch(QByteArrayLiteral("images_dark"));

    package.removeDefinition("preferred");
    package.addFileDefinition("preferred", QStringLiteral("images/") + preferred, QStringLiteral("Recommended wallpaper file"));

    if (!preferredDark.isEmpty()) {
        package.removeDefinition("preferredDark");
        package.addFileDefinition("preferredDark",
                                  QStringLiteral("images_dark%1").arg(QDir::separator()) + preferredDark,
                                  QStringLiteral("Recommended dark wallpaper file"));
    }
}

QString parsePreferredImage(const QString &absoluteFilePath, const QSize &targetSize)
{
    // Convert baseName to regex
    const QFileInfo info(absoluteFilePath);
    QString baseName = info.baseName();

    if (!baseName.contains(QLatin1String("${resolution}"))) {
        // No resolution variants
        return absoluteFilePath;
    }

    // Escape all reserved characters used by regex
    const char escapedChars[] = ",^$.[]*?+{}|()-";
    for (const char *c = escapedChars; *c; c++) {
        baseName.replace(QLatin1Char(*c), QLatin1String("\\%1").arg(*c));
    }

    QRegularExpression regExp;
    regExp.setPattern(baseName.replace(QLatin1String("\\$\\{resolution\\}"), QLatin1String("(.+)")));

    auto findBestMatch = [&](const QDir &parentDir) {
        const auto images = parentDir.entryInfoList();
        if (images.empty()) {
            return absoluteFilePath;
        }

        float best = std::numeric_limits<float>::max();
        QString preferred;

        for (const QFileInfo &entry : images) {
            const QRegularExpressionMatch result = regExp.match(entry.baseName());
            if (!result.hasMatch()) {
                continue;
            }

            const float dist = distance(resSize(result.captured(1)), targetSize);

            if (preferred.isEmpty() || dist < best) {
                preferred = entry.absoluteFilePath();
                best = dist;
            }
        }

        return preferred;
    };

    return findBestMatch(info.absoluteDir());
}

ImagePackage::ImagePackage()
    : Package(nullptr)
{
}

ImagePackage::ImagePackage(const Package &package, const QSize &targetSize)
    : Package(package)
    , m_targetSize(targetSize.isEmpty() ? QSize(1920, 1080) : targetSize)
{
    if (!package.isValid()) {
        return;
    }

    const QJsonObject rawData = metadata().rawData();
    auto it = rawData.constFind(QLatin1String("X-KDE-PlasmaImageWallpaper-Dynamic"));
    if (it != rawData.constEnd() && it->isObject()) {
        readDynamicFieldData(*it, package.path());
    } else {
        findPreferredImage(*this, m_targetSize);
        m_preferred = fileUrl(QByteArrayLiteral("preferred"));
        m_preferredDark = fileUrl(QByteArrayLiteral("preferredDark"));
    }
}

bool ImagePackage::isValid() const
{
    if (!metadata().isValid()) {
        return false;
    }

    if (m_dynamicType == DynamicType::None) {
        // Check if there are any available images.
        QDir imageDir(filePath("images"));
        imageDir.setFilter(QDir::Files | QDir::Readable);
        imageDir.setNameFilters(suffixes());

        if (imageDir.entryInfoList().empty()) {
            return false;
        }
    }

    if (m_dynamicType != DynamicType::None && m_dynamicMetadata.size() == 0) {
        return false;
    }

    return Package::isValid();
}

QUrl ImagePackage::preferred() const
{
    return m_preferred;
}

QUrl ImagePackage::preferredDark() const
{
    return m_preferredDark;
}

DynamicType::Type ImagePackage::dynamicType() const
{
    return m_dynamicType;
}

QDateTime ImagePackage::startTime() const
{
    return m_startTime;
}

std::size_t ImagePackage::dynamicMetadataSize() const
{
    return m_dynamicMetadata.size();
}

const DynamicMetadataItem &ImagePackage::dynamicMetadataAtIndex(std::size_t index) const
{
    return m_dynamicMetadata.at(index);
}

DynamicMetadataTimeInfoListPair ImagePackage::dynamicTimeList() const
{
    if (!m_timeInfoList.empty()) {
        return {m_timeInfoList, m_cycleTime};
    }

    for (const auto &item : std::as_const(m_dynamicMetadata)) {
        m_timeInfoList.emplace_back(DynamicMetadataTimeInfo{item.type, m_cycleTime});
        m_cycleTime += item.duration;
    }

    // The last item is used to help determine the current index
    m_timeInfoList.emplace_back(DynamicMetadataTimeInfo{DynamicMetadataItem::Static, m_cycleTime});

    return {m_timeInfoList, m_cycleTime};
}

std::pair<int, int> ImagePackage::indexAndIntervalAtDateTime(const QDateTime &dateTime) const
{
    int index = -1, interval = std::numeric_limits<int>::max();

    if (m_dynamicMetadata.empty()) {
        return {index, interval};
    }

    if (m_timeInfoList.empty()) {
        dynamicTimeList();
    }

    if (m_cycleTime == 0) {
        return {index, interval};
    }

    const qint64 timeDiff = m_startTime.secsTo(dateTime);
    if (timeDiff < 0) {
        return {index, interval};
    }

    // Align to the remaining time
    const quint64 modTime = timeDiff % m_cycleTime;

    for (std::size_t i = 0; i < m_timeInfoList.size(); i++) {
        const auto &p = m_timeInfoList.at(i);

        if (p.accumulatedTime > modTime) {
            // The previous wallpaper item is what we want
            const quint64 remainingTime = p.accumulatedTime - modTime;
            index = i - 1;

            if (m_timeInfoList.at(index).type == DynamicMetadataItem::Static) {
                // static, calculate remaining time, and make sure the least
                // interval is 1min
                interval = std::clamp<quint64>(remainingTime * 1000, 60 * 1000, std::numeric_limits<int>::max()); // sec to msec
            } else {
                // transition
                interval = std::min<quint64>(remainingTime, 600) * 1000;
            }

            break;
        }
    }

    return {index, interval};
}

void ImagePackage::readDynamicFieldData(const QJsonValue &value, const QString &packagePath)
{
    const QJsonObject wallpaperObject = value.toObject();
    if (wallpaperObject.empty()) {
        return;
    }

    // Dynamic type
    {
        const auto it = wallpaperObject.constFind(QLatin1String("Type"));
        if (it == wallpaperObject.constEnd() || it->type() != QJsonValue::String) {
            m_dynamicType = DynamicType::Timed;
        } else if (it->toString().compare(QLatin1String("solar"), Qt::CaseInsensitive) == 0) {
            m_dynamicType = DynamicType::Solar;
        } else if (it->toString().compare(QLatin1String("day-night"), Qt::CaseInsensitive) == 0
                   || it->toString().compare(QLatin1String("daynight"), Qt::CaseInsensitive) == 0) {
            m_dynamicType = DynamicType::DayNight;
        } else {
            m_dynamicType = DynamicType::Timed;
        }
    }

    // Start time
    if (m_dynamicType == DynamicType::Timed) {
        m_startTime = parseStartDateTime(wallpaperObject);
    }

    // Metadata
    if (m_dynamicType == DynamicType::Solar) {
        m_dynamicMetadata = parseSolarMetadata(wallpaperObject, packagePath, m_targetSize);
    } else if (m_dynamicType == DynamicType::DayNight) {
        m_dynamicMetadata = parseDayNightMetadata(wallpaperObject, packagePath, m_targetSize);
    } else if (m_dynamicType == DynamicType::Timed) {
        m_dynamicMetadata = parseTimedMetadata(wallpaperObject, packagePath, m_targetSize);
    }
}
}
