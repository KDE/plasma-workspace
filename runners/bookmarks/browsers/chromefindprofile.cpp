/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
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


#include "chromefindprofile.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QDebug>
#include "bookmarksrunner_defs.h"
#include "faviconfromblob.h"
#include <QFileInfo>

FindChromeProfile::FindChromeProfile (const QString &applicationName, const QString &homeDirectory, QObject* parent )
    : QObject(parent), m_applicationName(applicationName), m_homeDirectory(homeDirectory)
{
}

QList<Profile> FindChromeProfile::find()
{
  QString configDirectory = QStringLiteral("%1/.config/%2")
            .arg(m_homeDirectory, m_applicationName);
  QString localStateFileName = QStringLiteral("%1/Local State")
          .arg(configDirectory);

  QList<Profile> profiles;

  QFile localStateFile(localStateFileName);

  if (!localStateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return profiles;
  }
  QJsonDocument jdoc = QJsonDocument::fromJson(localStateFile.readAll());

  if(jdoc.isNull()) {
      qDebug() << "error opening " << QFileInfo(localStateFile).absoluteFilePath();
      return profiles;
  }

  QVariantMap localState = jdoc.object().toVariantMap();
  QVariantMap profilesConfig = localState.value(QStringLiteral("profile")).toMap().value(QStringLiteral("info_cache")).toMap();

  for(const QString &profile : profilesConfig.keys()) {
      const QString profilePath = QStringLiteral("%1/%2").arg(configDirectory, profile);
      const QString profileBookmarksPath = QStringLiteral("%1/%2").arg(profilePath, QStringLiteral("Bookmarks"));
      profiles << Profile(profileBookmarksPath, profile, FaviconFromBlob::chrome(profilePath, this));
  }

  return profiles;
}
