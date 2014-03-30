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

#ifndef WALLPAPERSTRUCTURE_H
#define WALLPAPERSTRUCTURE_H

#include <Plasma/PackageStructure>

#include "image.h"

class WallpaperPackage : public Plasma::PackageStructure
{
    Q_OBJECT

public:
    explicit WallpaperPackage(Image *paper, QObject *parent = 0, const QVariantList &args = QVariantList());

    void initPackage(Plasma::Package *package);
    void pathChanged(Plasma::Package *package);

protected:
    void pathChanged();

private:
    QSize resSize(const QString &str) const;
    void findBestPaper(Plasma::Package *package);
    float distance(const QSize& size, const QSize& desired) const;

private Q_SLOTS:
    void paperDestroyed();
    void renderHintsChanged();

private:
    Image *m_paper;
    bool m_fullPackage;
    QSize m_targetSize;
};

#endif
