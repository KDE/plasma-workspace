/*
 *  Copyright 2019 David Redondo <kde@david-redondo.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

#include "slidemodel.h"

void SlideModel::reload(const QStringList &selected)
{
    if (!m_packages.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_packages.count() - 1);
        m_packages.clear();
        endRemoveRows();
        emit countChanged();
    }
    addDirs(selected);
}

void SlideModel::addDirs(const QStringList &selected)
{
    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), selected);
    connect(finder, &BackgroundFinder::backgroundsFound, this, &SlideModel::backgroundsFound);
    m_findToken = finder->token();
    finder->start();
}

void SlideModel::backgroundsFound(const QStringList &paths, const QString &token)
{
    if (token != m_findToken) {
        return;
    }
    processPaths(paths);
    emit done();
}

void SlideModel::removeDir(const QString &path)
{
    BackgroundFinder *finder = new BackgroundFinder(m_wallpaper.data(), QStringList{path});
    connect(finder, &BackgroundFinder::backgroundsFound, this, &SlideModel::removeBackgrounds);
    finder->start();
}

void SlideModel::removeBackgrounds(const QStringList &paths, const QString &token)
{
    for (const QString &file : paths) {
        removeBackground(file);
    }
}

QVariant SlideModel::data(const QModelIndex &index, int role) const
{
    if (role == ToggleRole) {
        return !m_wallpaper.data()->uncheckedSlides().contains(data(index, PathRole).toString());
    }
    return BackgroundListModel::data(index, role);
}

QHash<int, QByteArray> SlideModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = BackgroundListModel::roleNames();
    roleNames.insert(ToggleRole, "checked");
    return roleNames;
}
