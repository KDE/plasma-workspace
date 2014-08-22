/*
 * Copyright 2014  Bhushan Shah <bhush94@gmail.com>
 * Copyright 2014 Marco Martin <notmart@gmail.com>
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

#include <QUrl>
#include <QQmlEngine>
#include <QQmlContext>

#include <QDebug>
#include <Plasma/Package>
#include <KLocalizedString>

#include "plasmawindowedview.h"

PlasmaWindowedView::PlasmaWindowedView(PlasmaWindowedCorona *corona, QWindow *parent)
    : PlasmaQuick::View(corona, parent)
{
    setTitle(i18n("Plasma Windowed"));
    corona->setView(this);
    engine()->rootContext()->setContextProperty("desktop", this);
    setSource(QUrl::fromLocalFile(corona->package().filePath("views", "Desktop.qml")));
}

PlasmaWindowedView::~PlasmaWindowedView()
{
}

#include "plasmawindowedview.moc"
