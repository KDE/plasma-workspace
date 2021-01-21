/*
 *   Copyright 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
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

#include "falkon.h"
#include "favicon.h"
#include "faviconfromblob.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

Falkon::Falkon(QObject *parent)
    : QObject(parent)
    , m_startupProfile(getStartupProfileDir())
    , m_favicon(FaviconFromBlob::falkon(m_startupProfile, this))
{
}

QList<BookmarkMatch> Falkon::match(const QString &term, bool addEverything)
{
    QList<BookmarkMatch> matches;
    for (const auto &bookmark : qAsConst(m_falkonBookmarkEntries)) {
        const auto obj = bookmark.toObject();
        const QString url = obj.value(QStringLiteral("url")).toString();
        BookmarkMatch bookmarkMatch(m_favicon->iconFor(url), term, obj.value(QStringLiteral("name")).toString(), url);
        bookmarkMatch.addTo(matches, addEverything);
    }
    return matches;
}

void Falkon::prepare()
{
    m_falkonBookmarkEntries = readChromeFormatBookmarks(m_startupProfile + QStringLiteral("/bookmarks.json"));
}

void Falkon::teardown()
{
    m_falkonBookmarkEntries = QJsonArray();
}

QString Falkon::getStartupProfileDir()
{
    const QString profilesIni = QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("/falkon/profiles/profiles.ini"));
    const QString startupProfile = KSharedConfig::openConfig(profilesIni)->group("Profiles").readEntry("startProfile", QStringLiteral("default"));
    return QFileInfo(profilesIni).dir().absoluteFilePath(startupProfile);
}
