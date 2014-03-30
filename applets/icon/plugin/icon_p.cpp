/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "icon_p.h"

#include <QFileInfo>

#include <KFileItem>
#include <KDesktopFile>
#include <QMimeType>
#include <QMimeDatabase>

#include <kio/global.h>

IconPrivate::IconPrivate() {
}

IconPrivate::~IconPrivate() {
}

void IconPrivate::setUrl(QUrl& url) {

    m_url = url;

    if (m_url.isLocalFile()) {

	const KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, m_url);
	const QFileInfo fi(m_url.toLocalFile());

	if (fileItem.isDesktopFile()) {
	    const KDesktopFile f(m_url.toLocalFile());
	    m_name = f.readName();
	    m_icon = f.readIcon();
	    m_genericName = f.readGenericName();
	    if (m_name.isNull()) {
		m_name = QFileInfo(m_url.toLocalFile()).fileName();
	    }
	} else {
	    QMimeDatabase db;
	    m_name = fi.baseName();
	    m_icon = db.mimeTypeForUrl(m_url).iconName();
	    m_genericName = fi.baseName();
	}
    } else {
	if (m_url.scheme().contains("http")) {
	    m_name = m_url.host();
	} else if (m_name.isEmpty()) {
	    m_name = m_url.toString();
	    if (m_name.endsWith(QLatin1String(":/"))) {
		m_name = m_url.scheme();
	    }
	}
	m_icon = KIO::iconNameForUrl(url);
    }

    emit urlChanged(m_url);
    emit nameChanged(m_name);
    emit iconChanged(m_icon);
    emit genericNameChanged(m_genericName);

}

QUrl IconPrivate::url() const
{
    return m_url;
}

QString IconPrivate::name() const
{
    return m_name;
}

QString IconPrivate::icon() const
{
    return m_icon;
}

QString IconPrivate::genericName() const
{
    return m_genericName;
}

#include "icon_p.moc"