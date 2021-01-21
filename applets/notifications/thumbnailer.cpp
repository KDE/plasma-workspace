/*
    Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "thumbnailer.h"

#include <KIO/PreviewJob>

#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KLocalizedString>
#include <KPropertiesDialog>
#include <KProtocolManager>
#include <KUrlMimeData>

#include <KIO/OpenFileManagerWindowJob>

Thumbnailer::Thumbnailer(QObject *parent)
    : QObject(parent)
{
}

Thumbnailer::~Thumbnailer() = default;

void Thumbnailer::classBegin()
{
}

void Thumbnailer::componentComplete()
{
    m_inited = true;
    generatePreview();
}

QUrl Thumbnailer::url() const
{
    return m_url;
}

void Thumbnailer::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        emit urlChanged();

        generatePreview();
    }
}

QSize Thumbnailer::size() const
{
    return m_size;
}

void Thumbnailer::setSize(const QSize &size)
{
    if (m_size != size) {
        m_size = size;
        emit sizeChanged();

        generatePreview();
    }
}

bool Thumbnailer::busy() const
{
    return m_busy;
}

bool Thumbnailer::hasPreview() const
{
    return !m_pixmap.isNull();
}

QPixmap Thumbnailer::pixmap() const
{
    return m_pixmap;
}

QSize Thumbnailer::pixmapSize() const
{
    return m_pixmap.size();
}

QString Thumbnailer::iconName() const
{
    return m_iconName;
}

bool Thumbnailer::menuVisible() const
{
    return m_menuVisible;
}

void Thumbnailer::generatePreview()
{
    if (!m_inited) {
        return;
    }

    if (!m_url.isValid() || !m_url.isLocalFile() || !m_size.isValid()) {
        return;
    }

    auto maxSize = qMax(m_size.width(), m_size.height());
    KIO::PreviewJob *job = KIO::filePreview(KFileItemList({KFileItem(m_url)}), QSize(maxSize, maxSize));
    job->setScaleType(KIO::PreviewJob::Scaled);
    job->setIgnoreMaximumSize(true);

    connect(job, &KIO::PreviewJob::gotPreview, this, [this](const KFileItem &item, const QPixmap &preview) {
        Q_UNUSED(item);
        m_pixmap = preview;
        emit pixmapChanged();

        if (!m_iconName.isEmpty()) {
            m_iconName.clear();
            emit iconNameChanged();
        }
    });

    connect(job, &KIO::PreviewJob::failed, this, [this](const KFileItem &item) {
        m_pixmap = QPixmap();
        emit pixmapChanged();

        const QString &iconName = item.determineMimeType().iconName();
        if (m_iconName != iconName) {
            m_iconName = iconName;
            emit iconNameChanged();
        }
    });

    connect(job, &KJob::result, this, [this] {
        m_busy = false;
        emit busyChanged();
    });

    m_busy = true;
    emit busyChanged();

    job->start();
}
