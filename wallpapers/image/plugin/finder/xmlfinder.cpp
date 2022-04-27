/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlfinder.h"

#include <QCollator>
#include <QDir>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include "distance.h"
#include "findsymlinktarget.h"
#include "suffixcheck.h"

XmlFinder::XmlFinder(const QStringList &paths, const QSize &targetSize, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_targetSize(targetSize)
{
}

void XmlFinder::run()
{
    QStringList xmls;

    QDir dir;
    dir.setFilter(QDir::AllDirs | QDir::Files | QDir::Readable);
    dir.setNameFilters({QStringLiteral("*.xml")});

    // m_paths is mutable
    int i = 0;
    while (i < m_paths.size()) {
        const QString &path = m_paths.at(i);

        // Read saved links from the configuration
        if (QUrl url(path); url.scheme() == QStringLiteral("image") && url.host() == QStringLiteral("gnome-wp-list")) {
            const QUrlQuery urlQuery(url);
            const QString root = urlQuery.queryItemValue(QStringLiteral("_root"));

            if (QFileInfo info(root); info.suffix().toLower() == QStringLiteral("xml") && info.isFile() && !info.isHidden()) {
                xmls.append(root);
            }

            continue;
        }

        // Is an xml file
        if (QFileInfo info(path); path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive) && info.isFile()) {
            xmls.append(findSymlinkTarget(info));
            continue;
        }

        dir.setPath(path);
        const QFileInfoList files = dir.entryInfoList();

        for (const QFileInfo &wp : files) {
            if (wp.isFile()) {
                xmls.append(findSymlinkTarget(wp));
            } else {
                const QString name = wp.fileName();

                if (name.startsWith(QLatin1Char('.'))) {
                    continue;
                }

                // add this to the directories we should be looking at
                m_paths.append(wp.filePath());
            }
        }

        ++i;
    }

    xmls.removeDuplicates();

    QList<WallpaperItem> packages;

    for (const QString &path : std::as_const(xmls)) {
        packages << parseXml(path, m_targetSize);
    }

    sort(packages);

    Q_EMIT xmlFound(packages);
}

void XmlFinder::sort(QList<WallpaperItem> &list)
{
    QCollator collator;

    // Make sure 2 comes before 10
    collator.setNumericMode(true);
    // Behave like Dolphin with natural sorting enabled
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    const auto compare = [&collator](const WallpaperItem &a, const WallpaperItem &b) {
        // Checking if less than zero makes ascending order (A-Z)
        return collator.compare(a.name, b.name) < 0;
    };

    std::stable_sort(list.begin(), list.end(), compare);
}

QList<WallpaperItem> XmlFinder::parseXml(const QString &path, const QSize &targetSize)
{
    QList<WallpaperItem> results;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return results;
    }

    QXmlStreamReader xml(&file);

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QStringLiteral("wallpaper")) {
            WallpaperItem item;

            while (!xml.atEnd()) {
                xml.readNext();

                if (xml.isEndElement()) {
                    if (xml.name() == QStringLiteral("wallpaper")) {
                        const QFileInfo info(item.filename);
                        // Check is acceptable suffix
                        if (!info.isFile() || !(info.suffix().toLower() == QStringLiteral("xml") || isAcceptableSuffix(info.suffix()))) {
                            break;
                        }

                        if (item.name.isEmpty()) {
                            item.name = info.baseName();
                        }

                        item._root = path;
                        item.path = convertToUrl(item);

                        results.append(item);
                        break;
                    } else {
                        continue;
                    }
                }

                if (xml.name() == QStringLiteral("name")) {
                    /* no pictures available for the specified parameters */
                    item.name = xml.readElementText();
                } else if (xml.name() == QStringLiteral("filename")) {
                    item.filename = xml.readElementText();

                    if (QFileInfo(item.filename).isRelative()) {
                        item.filename = QFileInfo(path).absoluteDir().absoluteFilePath(item.filename);
                    }

                    if (item.filename.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
                        item.slideshow = parseSlideshowXml(item.filename, targetSize);
                    }
                } else if (xml.name() == QStringLiteral("filename-dark")) {
                    item.filename_dark = xml.readElementText();

                    if (QFileInfo(item.filename_dark).isRelative()) {
                        item.filename_dark = QFileInfo(path).absoluteDir().absoluteFilePath(item.filename_dark);
                    }
                } else if (xml.name() == QStringLiteral("author")) {
                    item.author = xml.readElementText();
                }
            }
        }
    }

    XmlFinder::sort(results);

    return results;
}

SlideshowData XmlFinder::parseSlideshowXml(const QString &path, const QSize &targetSize)
{
    SlideshowData data;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return data;
    }

    QXmlStreamReader xml(&file);
    QXmlStreamReader::TokenType token;

    while (!xml.atEnd()) {
        token = xml.readNext();

        if (token == QXmlStreamReader::Comment) {
            continue;
        }

        if (xml.isStartElement() && xml.name() == QStringLiteral("background")) {
            while (!xml.atEnd()) { // background
                token = xml.readNext();

                if (token == QXmlStreamReader::Comment) {
                    continue;
                }

                if (xml.isEndElement()) {
                    if (xml.name() == QStringLiteral("background")) {
                        break;
                    } else {
                        continue;
                    }
                }

                if (xml.isStartElement()) {
                    if (xml.name() == QStringLiteral("starttime")) {
                        int year = QDate::currentDate().year(), month = QDate::currentDate().month(), day = QDate::currentDate().day();
                        int seconds = 0;

                        while (!xml.atEnd()) { // starttime
                            token = xml.readNext();

                            if (token == QXmlStreamReader::Comment) {
                                continue;
                            }

                            if (xml.isEndElement()) {
                                if (xml.name() == QStringLiteral("starttime")) {
                                    data.starttime.setDate(QDate(year, month, day));
                                    data.starttime = data.starttime.addSecs(seconds);
                                    break; // starttime
                                } else {
                                    continue;
                                }
                            }

                            if (xml.name() == QStringLiteral("year")) {
                                year = std::max(0, xml.readElementText().toInt());
                            } else if (xml.name() == QStringLiteral("month")) {
                                month = std::clamp(xml.readElementText().toInt(), 1, 12);
                            } else if (xml.name() == QStringLiteral("day")) {
                                day = std::clamp(xml.readElementText().toInt(), 1, 31);
                            } else if (xml.name() == QStringLiteral("hour")) {
                                seconds += xml.readElementText().toInt() * 3600;
                            } else if (xml.name() == QStringLiteral("minute")) {
                                seconds += xml.readElementText().toInt() * 60;
                            } else if (xml.name() == QStringLiteral("second")) {
                                seconds += xml.readElementText().toInt();
                            }
                        }
                    } else if (xml.name() == QStringLiteral("static")) {
                        SlideshowItemData sdata;
                        sdata.dataType = 0;

                        while (!xml.atEnd()) {
                            token = xml.readNext();

                            if (token == QXmlStreamReader::Comment) {
                                continue;
                            }

                            if (xml.isEndElement()) {
                                if (xml.name() == QStringLiteral("static")) {
                                    if (!sdata.file.isEmpty()) {
                                        data.data.append(sdata);
                                    }
                                    break;
                                } else {
                                    continue;
                                }
                            }

                            if (xml.name() == QStringLiteral("duration")) {
                                sdata.duration = xml.readElementText().toDouble();
                            } else if (xml.name() == QStringLiteral("file")) {
                                const QStringList results = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified().split(QLatin1Char(' '));

                                if (results.size() == 1) {
                                    sdata.file = results.at(0);
                                } else {
                                    sdata.file = findPreferredImage(results, targetSize);
                                }

                                if (QFileInfo(sdata.file).isRelative()) {
                                    sdata.file = QFileInfo(path).absoluteDir().absoluteFilePath(sdata.file);
                                }
                            }
                        }
                    } else if (xml.name() == QStringLiteral("transition")) {
                        SlideshowItemData tdata;
                        tdata.dataType = 1;

                        if (auto attr = xml.attributes(); attr.hasAttribute(QStringLiteral("type"))) {
                            tdata.type = attr.value(QStringLiteral("type")).toString();
                        }

                        while (!xml.atEnd()) { // static
                            token = xml.readNext();

                            if (token == QXmlStreamReader::Comment) {
                                continue;
                            }

                            if (xml.isEndElement()) {
                                if (xml.name() == QStringLiteral("transition")) {
                                    if (!tdata.from.isEmpty() && !tdata.to.isEmpty()) {
                                        data.data.append(tdata);
                                    }
                                    break; // static
                                } else {
                                    continue;
                                }
                            }

                            if (xml.name() == QStringLiteral("duration")) {
                                tdata.duration = xml.readElementText().toDouble();
                            } else if (xml.name() == QStringLiteral("from")) {
                                tdata.from = xml.readElementText();

                                if (QFileInfo(tdata.from).isRelative()) {
                                    tdata.from = QFileInfo(path).absoluteDir().absoluteFilePath(tdata.from);
                                }
                            } else if (xml.name() == QStringLiteral("to")) {
                                tdata.to = xml.readElementText();

                                if (QFileInfo(tdata.to).isRelative()) {
                                    tdata.to = QFileInfo(path).absoluteDir().absoluteFilePath(tdata.to);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (xml.isEndElement() && xml.name() == QStringLiteral("background")) {
            break;
        }
    }

    return data;
}

QUrl XmlFinder::convertToUrl(const WallpaperItem &item)
{
    QUrl url(QStringLiteral("image://gnome-wp-list/get"));

    QUrlQuery urlQuery(url);
    urlQuery.addQueryItem(QStringLiteral("_root"), item._root);
    urlQuery.addQueryItem(QStringLiteral("filename"), item.filename);
    urlQuery.addQueryItem(QStringLiteral("filename_dark"), item.filename_dark);
    urlQuery.addQueryItem(QStringLiteral("name"), item.name);
    urlQuery.addQueryItem(QStringLiteral("author"), item.author);

    // Parse slideshow data if the file is an xml file, no need to save them in the url.
    url.setQuery(urlQuery);

    return url;
}

QStringList XmlFinder::convertToPaths(const QUrl &url)
{
    if (url.scheme() != QStringLiteral("image") || url.host() != QStringLiteral("gnome-wp-list")) {
        return {};
    }

    const QUrlQuery urlQuery(url);

    const QString rootPath(urlQuery.queryItemValue(QStringLiteral("_root")));
    const QString filename(urlQuery.queryItemValue(QStringLiteral("filename")));

    return {rootPath, filename};
}

QString XmlFinder::findPreferredImage(const QStringList &pathList, const QSize &_targetSize)
{
    if (pathList.empty()) {
        return QString();
    }

    QSize targetSize = _targetSize;

    if (targetSize.isEmpty()) {
        targetSize = QSize(1920, 1080);
    }

    QString preferred;

    float best = std::numeric_limits<float>::max();

    for (const auto &p : pathList) {
        QSize candidate = resSize(QFileInfo(p).baseName());

        if (candidate.isEmpty()) {
            continue;
        }

        float dist = distance(candidate, targetSize);

        if (preferred.isEmpty() || dist < best) {
            preferred = p;
            best = dist;
        }
    }

    return preferred;
}
