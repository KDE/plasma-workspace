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

#include <PlasmaQuick/ConfigView>

namespace Plasma
{
class Containment;
}

class QAbstractItemModel;
class CurrentContainmentActionsModel;

namespace KDeclarative
{
class ConfigPropertyMap;
}

// TODO: out of the library?
class ContainmentConfigView : public PlasmaQuick::ConfigView
{
    Q_OBJECT
    Q_PROPERTY(PlasmaQuick::ConfigModel *containmentActionConfigModel READ containmentActionConfigModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *currentContainmentActionsModel READ currentContainmentActionsModel CONSTANT)
    Q_PROPERTY(PlasmaQuick::ConfigModel *wallpaperConfigModel READ wallpaperConfigModel CONSTANT)
    Q_PROPERTY(PlasmaQuick::ConfigModel *containmentPluginsConfigModel READ containmentPluginsConfigModel CONSTANT)
    Q_PROPERTY(KDeclarative::ConfigPropertyMap *wallpaperConfiguration READ wallpaperConfiguration NOTIFY wallpaperConfigurationChanged)
    Q_PROPERTY(QString currentWallpaper READ currentWallpaper WRITE setCurrentWallpaper NOTIFY currentWallpaperChanged)
    Q_PROPERTY(QString containmentPlugin READ containmentPlugin WRITE setContainmentPlugin NOTIFY containmentPluginChanged)

public:
    explicit ContainmentConfigView(Plasma::Containment *interface, QWindow *parent = nullptr);
    ~ContainmentConfigView() override;

    void init() override;

    PlasmaQuick::ConfigModel *containmentActionConfigModel();
    QAbstractItemModel *currentContainmentActionsModel();
    PlasmaQuick::ConfigModel *wallpaperConfigModel();
    PlasmaQuick::ConfigModel *containmentPluginsConfigModel();
    QString currentWallpaper() const;
    void setCurrentWallpaper(const QString &wallpaper);
    KDeclarative::ConfigPropertyMap *wallpaperConfiguration() const;
    QString containmentPlugin() const;
    void setContainmentPlugin(const QString &plugin);

    Q_INVOKABLE void applyWallpaper();

Q_SIGNALS:
    void currentWallpaperChanged();
    void wallpaperConfigurationChanged();
    void containmentPluginChanged();

protected:
    void syncWallpaperObjects();

private:
    Plasma::Containment *m_containment = nullptr;
    PlasmaQuick::ConfigModel *m_wallpaperConfigModel = nullptr;
    PlasmaQuick::ConfigModel *m_containmentActionConfigModel = nullptr;
    PlasmaQuick::ConfigModel *m_containmentPluginsConfigModel = nullptr;
    CurrentContainmentActionsModel *m_currentContainmentActionsModel = nullptr;
    QString m_currentWallpaper;
    KDeclarative::ConfigPropertyMap *m_currentWallpaperConfig = nullptr;
    KDeclarative::ConfigPropertyMap *m_ownWallpaperConfig = nullptr;
};

#endif // multiple inclusion guard
