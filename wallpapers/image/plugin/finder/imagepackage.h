/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IMAGEPACKAGE_H
#define IMAGEPACKAGE_H

#include <QObject>
#include <QSize>
#include <QTime>

#include <KPackage/Package>

#include "utils/dynamictype.h"

class QJsonValue;

struct DynamicMetadataItem {
    enum Type {
        Static,
        Transition,
    };
    Type type = Static;

    enum TimeOfDay {
        Day,
        Night,
    };
    TimeOfDay timeOfDay = Day;

    QString filename;

    // For "timed"
    quint64 duration = 3600; // Default 1h

    // For "solar"
    double solarAzimuth = 0.0;
    double solarElevation = 0.0;
    QTime startTime;
};

struct DynamicMetadataTimeInfo {
    DynamicMetadataItem::Type type = DynamicMetadataItem::Static;
    quint64 accumulatedTime = 0;
};

using DynamicMetadataItems = std::vector<DynamicMetadataItem>;
using DynamicMetadataTimeInfoList = std::vector<DynamicMetadataTimeInfo>;
using DynamicMetadataTimeInfoListPair = std::pair<DynamicMetadataTimeInfoList, quint64 /* total time */>;

namespace KPackage
{
class ImagePackage;

// Expose functions for test purpose
QDateTime parseStartDateTime(const QJsonObject &wallpaperObject);

std::vector<DynamicMetadataItem> parseSolarMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize);
std::vector<DynamicMetadataItem> parseDayNightMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize);
std::vector<DynamicMetadataItem> parseTimedMetadata(const QJsonObject &wallpaperObject, const QString &packagePath, const QSize &targetSize);

/**
 * Finds the preferred image under the specific resolution
 */
void findPreferredImage(ImagePackage &package, const QSize &targetSize);

/**
 * Finds the preferred image under the specific resolution, but filename follows
 * a pattern like @c "dark-${resolution}.png".
 *
 * @return absolute path of the preferred image
 */
QString parsePreferredImage(const QString &absoluteFilePath, const QSize &targetSize);

/**
 * Inherits the traditional KPackage to support dark wallpaper and dynamic wallpaper.
 *
 * A dynamic wallpaper has several fields in @c X-KDE-PlasmaImageWallpaper-Dynamic,
 * including:
 *
 * - @param Type: pattern follwed by the dynamic wallpaper. Currently it supports
 *                @c "timed" (based on time) or @c "solar" (based on solar).
 *   @default "timed"
 *
 * - @param StartTime: the start time for the time-based dynamic wallpaper. The
 *                     wallpaper can specify @c Year, @c Month, @c Day, @c Hour,
 *                     @c Minute and @c Second.
 *   @default the start time of the current date
 *   @note If the specified date is invalid, the day before the current day will be
 *         used. If the specified time is invalid, 00:00:00 will be used.
 *
 * - @param Metadata a list of data fields, each field contains one image path, duration
 *                   or solar angles.
 *   For both patterns:
 *   - @param FileName the file path of the image relative to the metadata.json file.
 *     @note @c ${resolution} can be used in the file name to specify the position of
 *           the image resolution in the file name. The format is "<width>x<height>"
 *   - @param Type whether the current image is @c "static", or @c "transition" type
 *                 that should be blended with the last image.
 *
 *   For "time" pattern:
 *   - @param Duration duration in seconds to display the image. For transition type,
 *                     this value means how long the current image will fully replace
 *                     the last image.
 *
 *   For "solar" pattern:
 *   - @param SolarAzimuth the angle between a line due south and the shadow cast by a
 *                         vertical rod on Earth.
 *   - @param SolarElevation the angle between the imaginary horizontal plane and the
 *                           sun in the sky.
 *   - @param StartTime fallback time when the user is located near the North or the
 *                      South Pole.
 *
 * Example:
 * @code{.json}
    "X-KDE-PlasmaImageWallpaper-Dynamic": {
        "Type": "timed",
        "StartTime": {
            "Year": 2022,
            "Month": 5,
            "Day": 24,
            "Hour": 8,
            "Minute": 0,
            "Second": 0
        },
        "Metadata": [
            {
                "Duration": 36000,
                "FileName": "images/day-${resolution}.png",
                "Type": "static"
            },
            {
                "Duration": 7200,
                "FileName": "images/night-${resolution}.png",
                "Type": "transition"
            },
            {
                "Duration": 36000,
                "FileName": "images/night-${resolution}.png",
                "Type": "static"
            },
            {
                "Duration": 7200,
                "FileName": "images/day-${resolution}.png",
                "Type": "transition"
            }
        ]
    }
   @endcode

   @code{.json}
    "X-KDE-PlasmaImageWallpaper-Dynamic": {
        "Type": "solar",
        "Metadata": [
            {
                "FileName": "images/day-${resolution}.png",
                "SolarAzimuth": 120,
                "SolarElevation": 30,
                "StartTime": {
                    "Hour": 8,
                    "Minute": 0,
                    "Second": 0
                },
                "Type": "static"
            },
            {
                "FileName": "images/night-${resolution}.png",
                "SolarAzimuth": 270,
                "SolarElevation": 0,
                "StartTime": {
                    "Hour": 18,
                    "Minute": 0,
                    "Second": 0
                },
                "Type": "transition"
            },
            {
                "FileName": "images/night-${resolution}.png",
                "SolarAzimuth": 300,
                "SolarElevation": -30,
                "StartTime": {
                    "Hour": 20,
                    "Minute": 0,
                    "Second": 0
                },
                "Type": "static"
            },
            {
                "FileName": "images/day-${resolution}.png",
                "SolarAzimuth": 90,
                "SolarElevation": 0,
                "StartTime": {
                    "Hour": 6,
                    "Minute": 0,
                    "Second": 0
                },
                "Type": "transition"
            }
        ]
    }
   @endcode
 */
class ImagePackage : public KPackage::Package
{
public:
    /**
     * Creates an empty KPackage.
     */
    explicit ImagePackage();

    /**
     * Extracts @c preferred and @c preferredDark URLs from the content
     * of @c package.
     */
    explicit ImagePackage(const Package &package, const QSize &targetSize);

    bool isValid() const;

    QUrl preferred() const;
    QUrl preferredDark() const;

    DynamicType::Type dynamicType() const;

    /**
     * Start time of the timed dynamic wallpaper
     */
    QDateTime startTime() const;

    /**
     * @return the size of dynamic metadata
     */
    std::size_t dynamicMetadataSize() const;

    /**
     * @return dynamic wallpaper metadata
     */
    const DynamicMetadataItem &dynamicMetadataAtIndex(std::size_t index) const;

    /**
     * @return @c first time information list for timed dynamic wallpaper
     *         @c second total time of a slideshow cycle
     */
    DynamicMetadataTimeInfoListPair dynamicTimeList() const;

    /**
     * @return @c first index of the wallpaper item at the specified time
     *         @c second remaining time to the next wallpaper item
     */
    std::pair<int, int> indexAndIntervalAtDateTime(const QDateTime &dateTime) const;

private:
    void readDynamicFieldData(const QJsonValue &value, const QString &packagePath);

    QSize m_targetSize;
    QUrl m_preferred;
    QUrl m_preferredDark;

    DynamicType::Type m_dynamicType = DynamicType::None;
    DynamicMetadataItems m_dynamicMetadata;

    // Define the start time for "timed" pattern
    QDateTime m_startTime;
    mutable DynamicMetadataTimeInfoList m_timeInfoList;
    mutable quint64 m_cycleTime = 0;
};
}
Q_DECLARE_METATYPE(KPackage::ImagePackage)

#endif // IMAGEPACKAGE_H
