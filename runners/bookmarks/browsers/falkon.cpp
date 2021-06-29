/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    const QString startupProfile = KSharedConfig::openConfig(profilesIni)->group("Profiles").readEntry("startProfile", QStringLiteral("default")).remove("\"");
    return QFileInfo(profilesIni).dir().absoluteFilePath(startupProfile);
}
