/*
 *   Copyright (C) 2018 Chris Holland <zrenfire@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WALLPAPERPLUGIN_H
#define WALLPAPERPLUGIN_H

#include <QObject>
#include <QPointer>

class QQuickItem;

namespace KNS3 {
    class DownloadDialog;
}

class WallpaperPlugin : public QObject
{
    Q_OBJECT

    public:
        explicit WallpaperPlugin(QObject* parent = nullptr);
        ~WallpaperPlugin() override;

        Q_INVOKABLE void getNewWallpaperPlugin(QQuickItem *ctx = nullptr);

    private:
        QPointer<KNS3::DownloadDialog> m_newStuffDialog;
};

#endif
