/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
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

#ifndef CONTAINMENTCONFIGVIEW_H
#define CONTAINMENTCONFIGVIEW_H


#include "configview.h"


namespace Plasma {
    class Containment;
}

class ConfigPropertyMap;
class CurrentContainmentActionsModel;


class ContainmentConfigView : public ConfigView
{
    Q_OBJECT
    Q_PROPERTY(ConfigModel *containmentActionConfigModel READ containmentActionConfigModel CONSTANT)
    Q_PROPERTY(QStandardItemModel *currentContainmentActionsModel READ currentContainmentActionsModel CONSTANT)
    Q_PROPERTY(ConfigModel *wallpaperConfigModel READ wallpaperConfigModel CONSTANT)
    Q_PROPERTY(ConfigPropertyMap *wallpaperConfiguration READ wallpaperConfiguration NOTIFY wallpaperConfigurationChanged)
    Q_PROPERTY(QString currentWallpaper READ currentWallpaper WRITE setCurrentWallpaper NOTIFY currentWallpaperChanged)

public:
    ContainmentConfigView(Plasma::Containment *interface, QWindow *parent = 0);
    virtual ~ContainmentConfigView();

    virtual void init();

    ConfigModel *containmentActionConfigModel();
    QStandardItemModel *currentContainmentActionsModel();
    ConfigModel *wallpaperConfigModel();
    QString currentWallpaper() const;
    void setCurrentWallpaper(const QString &wallpaper);
    ConfigPropertyMap *wallpaperConfiguration() const;

    Q_INVOKABLE void applyWallpaper();

Q_SIGNALS:
    void currentWallpaperChanged();
    void wallpaperConfigurationChanged();

protected:
    void syncWallpaperObjects();

private:
    Plasma::Containment *m_containment;
    ConfigModel *m_wallpaperConfigModel;
    ConfigModel *m_containmentActionConfigModel;
    CurrentContainmentActionsModel *m_currentContainmentActionsModel;
    QString m_currentWallpaper;
    ConfigPropertyMap *m_currentWallpaperConfig;
    ConfigPropertyMap *m_ownWallpaperConfig;
};

#endif // multiple inclusion guard
