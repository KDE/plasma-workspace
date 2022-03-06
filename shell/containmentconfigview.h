/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <PlasmaQuick/ConfigModel>
#include <PlasmaQuick/ConfigView>
#include <QQmlPropertyMap>

namespace Plasma
{
class Containment;
}

class QAbstractItemModel;
class CurrentContainmentActionsModel;

// TODO: out of the library?
class ContainmentConfigView : public PlasmaQuick::ConfigView
{
    Q_OBJECT
    Q_PROPERTY(PlasmaQuick::ConfigModel *containmentActionConfigModel READ containmentActionConfigModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *currentContainmentActionsModel READ currentContainmentActionsModel CONSTANT)
    Q_PROPERTY(PlasmaQuick::ConfigModel *wallpaperConfigModel READ wallpaperConfigModel CONSTANT)
    Q_PROPERTY(PlasmaQuick::ConfigModel *containmentPluginsConfigModel READ containmentPluginsConfigModel CONSTANT)
    Q_PROPERTY(QQmlPropertyMap *wallpaperConfiguration READ wallpaperConfiguration NOTIFY wallpaperConfigurationChanged)
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
    QQmlPropertyMap *wallpaperConfiguration() const;
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
    QQmlPropertyMap *m_currentWallpaperConfig = nullptr;
    QQmlPropertyMap *m_ownWallpaperConfig = nullptr;
};
