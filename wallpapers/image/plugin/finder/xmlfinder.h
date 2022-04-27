/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef XMLFINDER_H
#define XMLFINDER_H

#include <QDateTime>
#include <QObject>
#include <QRunnable>
#include <QSize>
#include <QUrl>

/*
   Slideshow format:
    <background>
    <starttime>
        <year>2021</year>
        <month>4</month>
        <day>2</day>
        <hour>13</hour>
        <minute>14</minute>
        <second>15</second>
    </starttime>

    <static>
        <duration>60.0</duration>
        <file>start.png</file>
    </static>

    <transition type="overlay">
        <duration>1261440000.0</duration>
        <from>@datadir@/backgrounds/gnome/adwaita-morning.jpg</from>
        <to>@datadir@/backgrounds/gnome/adwaita-day.jpg</to>
    </transition>

    <static>
        <duration>60.0</duration>
        <file>end.png</file>
    </static>

    </background>
 */

struct SlideshowItemData {
    int dataType;
    quint64 duration; // unit: sec

    QString file;

    QString type;
    QString from;
    QString to;
};

/**
 * This stores slideshow items.
 * The xml file often contains:
 * - <starttime>
 * - <transition>
 * - <static>
 */
struct SlideshowData {
    QDateTime starttime;
    QList<SlideshowItemData> data;
};

/*
   Wallpaper list format:
    <?xml version="1.0"?>
    <!DOCTYPE wallpapers SYSTEM "gnome-wp-list.dtd">
    <wallpapers>
    <wallpaper deleted="false">
        <name>Default Background</name>
        <filename>@BACKGROUNDDIR@/adwaita-l.jpg</filename>
        <filename-dark>@BACKGROUNDDIR@/adwaita-d.jpg</filename-dark>
        <options>zoom</options>
        <shade_type>solid</shade_type>
        <pcolor>#3465a4</pcolor>
        <scolor>#000000</scolor>
    </wallpaper>
    </wallpapers>
 */

/**
 * This stores the wallpaper item.
 */
struct WallpaperItem {
    QString _root;
    QUrl path; // image://gnome-wp-list/get?...
    QString filename; // The path can be an xml file or an image file
    QString filename_dark;
    QString name;
    QString author;
    SlideshowData slideshow;
};
Q_DECLARE_METATYPE(WallpaperItem)

/**
 * A runnable that finds XML wallpapers.
 */
class XmlFinder : public QObject, public QRunnable
{
    Q_OBJECT

public:
    XmlFinder(const QStringList &paths, const QSize &targetSize, QObject *parent = nullptr);

    void run() override;

    static void sort(QList<WallpaperItem> &list);
    static QList<WallpaperItem> parseXml(const QString &path, const QSize &targetSize);
    static SlideshowData parseSlideshowXml(const QString &path, const QSize &targetSize);

    static QUrl convertToUrl(const WallpaperItem &item);
    static QStringList convertToPaths(const QUrl &url);

    static QString findPreferredImage(const QStringList &sizeList, const QSize &targetSize);

Q_SIGNALS:
    void xmlFound(const QList<WallpaperItem> &packages);

private:
    QStringList m_paths;
    QSize m_targetSize;
};

#endif // XMLFINDER_H
