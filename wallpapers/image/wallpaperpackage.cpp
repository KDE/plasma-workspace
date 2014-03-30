/*
 *   Copyright 2013 by Marco Martin <mart@kde.org>

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

#include "wallpaperpackage.h"

#include <math.h>
#include <float.h> // FLT_MAX

#include <QFileInfo>
#include <QDebug>

#include <klocalizedstring.h>

WallpaperPackage::WallpaperPackage(Image *paper, QObject *parent, const QVariantList &args)
    : Plasma::PackageStructure(parent, args),
      m_paper(paper),
      m_fullPackage(true),
      m_targetSize(100000, 100000)
{
}

void WallpaperPackage::initPackage(Plasma::Package *package)
{
    package->addDirectoryDefinition("images", "images/", i18n("Images"));

    QStringList mimetypes;
    mimetypes << "image/svg" << "image/png" << "image/jpeg" << "image/jpg";
    package->setMimeTypes("images", mimetypes);

    package->setRequired("images", true);
    package->addFileDefinition("screenshot", "screenshot.png", i18n("Screenshot"));
    package->setAllowExternalPaths(true);

    if (m_paper) {
        m_targetSize = m_paper->targetSize();

        connect(m_paper, SIGNAL(renderingModeChanged()), this, SLOT(renderHintsChanged()));
        connect(m_paper, SIGNAL(destroyed(QObject*)), this, SLOT(paperDestroyed()));
    }
}

void WallpaperPackage::renderHintsChanged()
{
    if (m_paper) {
        m_targetSize = m_paper->targetSize();

        if (m_fullPackage) {
            findBestPaper(m_paper->package());
        }
    }
}

void WallpaperPackage::pathChanged(Plasma::Package *package)
{
    static bool guard = false;

    if (guard) {
        return;
    }

    guard = true;
    QString ppath = package->path();
    if (ppath.endsWith('/')) {
        ppath.chop(1);
        if (!QFile::exists(ppath)) {
            ppath = package->path();
        }
    }
    QFileInfo info(ppath);
    m_fullPackage = info.isDir();
    package->removeDefinition("preferred");
    package->setRequired("images", m_fullPackage);

    if (m_fullPackage) {
        package->setContentsPrefixPaths(QStringList() << "contents/");
        findBestPaper(package);
    } else {
        // dirty trick to support having a file passed in instead of a directory
        package->addFileDefinition("preferred", info.fileName(), i18n("Recommended wallpaper file"));
        package->addFileDefinition("sceenshot", info.fileName(), i18n("Preview"));
        package->setContentsPrefixPaths(QStringList());
        package->setPath(info.path());
    }

    guard = false;
}

QSize WallpaperPackage::resSize(const QString &str) const
{
    int index = str.indexOf('x');
    if (index != -1) {
        return QSize(str.left(index).toInt(),
                     str.mid(index + 1).toInt());
    } else {
        return QSize();
    }
}

void WallpaperPackage::findBestPaper(Plasma::Package *package)
{
    //FIXME
    qWarning()<<"Why this affects entryList?"<<package->isValid();
    QStringList images = package->entryList("images");
    if (images.empty()) {
        return;
    }

    //qDebug() << "wanted" << size;

    // choose the nearest resolution
    float best = FLT_MAX;

    QString bestImage;
    foreach (const QString &entry, images) {
        QSize candidate = resSize(QFileInfo(entry).baseName());
        if (candidate == QSize()) {
            continue;
        }

        double dist = distance(candidate, m_targetSize);
        //qDebug() << "candidate" << candidate << "distance" << dist;
        if (bestImage.isEmpty() || dist < best) {
            bestImage = entry;
            best = dist;
            //qDebug() << "best" << bestImage;
            if (dist == 0) {
                break;
            }
        }
    }

    //qDebug() << "best image" << bestImage;
    package->removeDefinition("preferred");
    package->addFileDefinition("preferred", "images/" + bestImage, i18n("Recommended wallpaper file"));
}

float WallpaperPackage::distance(const QSize& size, const QSize& desired) const
{
    // compute difference of areas
    float delta = size.width() * size.height() -
                  desired.width() * desired.height();
    // scale down to about 1.0
    delta /= ((desired.width() * desired.height())+(size.width() * size.height()))/2;

    // Difference of areas, slight preference to scale down
    return delta >= 0.0 ? delta : -delta + 2.0;
}

void WallpaperPackage::paperDestroyed()
{
    m_paper = 0;
}


#include "wallpaperpackage.moc"

