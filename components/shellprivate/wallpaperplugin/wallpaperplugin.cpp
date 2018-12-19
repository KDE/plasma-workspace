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

#include "wallpaperplugin.h"

#include <QQuickItem>
#include <QQuickWindow>

#include <KNewStuff3/KNS3/DownloadDialog>
#include <klocalizedstring.h>

#include <Plasma/Theme>

WallpaperPlugin::WallpaperPlugin(QObject *parent)
    : QObject(parent)
{

}

WallpaperPlugin::~WallpaperPlugin()
{

}

void WallpaperPlugin::getNewWallpaperPlugin(QQuickItem *ctx)
{
    if (!m_newStuffDialog) {
        m_newStuffDialog = new KNS3::DownloadDialog( QString::fromLatin1("wallpaperplugin.knsrc") );
        m_newStuffDialog->setTitle(i18n("Download Wallpaper Plugins"));
    }

    if (ctx && ctx->window()) {
        m_newStuffDialog->setWindowModality(Qt::WindowModal);
        m_newStuffDialog->winId(); // so it creates the windowHandle();
        m_newStuffDialog->windowHandle()->setTransientParent(ctx->window());
    }

    m_newStuffDialog->show();
}
