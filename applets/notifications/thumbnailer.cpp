/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
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

    if (!m_url.isValid() || !m_url.isLocalFile() || !m_size.isValid() || m_size.isEmpty()) {
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
