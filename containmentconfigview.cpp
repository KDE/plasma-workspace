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

#include "containmentconfigview.h"
#include <Plasma/Containment>
#include "currentcontainmentactionsmodel.h"
#include <kdeclarative/configpropertymap.h>

#include <QDebug>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>

#include <KLocalizedString>

#include <Plasma/Corona>
#include <Plasma/ContainmentActions>
#include <Plasma/PluginLoader>





//////////////////////////////ContainmentConfigView
ContainmentConfigView::ContainmentConfigView(Plasma::Containment *cont, QWindow *parent)
    : ConfigView(cont, parent),
      m_containment(cont),
      m_wallpaperConfigModel(0),
      m_containmentActionConfigModel(0),
      m_currentContainmentActionsModel(0),
      m_currentWallpaperConfig(0),
      m_ownWallpaperConfig(0)
{
    qmlRegisterType<QStandardItemModel>();
    engine()->rootContext()->setContextProperty("configDialog", this);
    setCurrentWallpaper(cont->containment()->wallpaper());

    Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
    pkg.setDefaultPackageRoot("plasma/wallpapers");
    pkg.setPath(m_containment->wallpaper());
    QFile file(pkg.filePath("config", "main.xml"));
    KConfigGroup cfg = m_containment->config();
    cfg = KConfigGroup(&cfg, "Wallpaper");

    syncWallpaperObjects();
}

ContainmentConfigView::~ContainmentConfigView()
{
}

void ContainmentConfigView::init()
{
    setSource(QUrl::fromLocalFile(m_containment->containment()->corona()->package().filePath("containmentconfigurationui")));
}

ConfigModel *ContainmentConfigView::containmentActionConfigModel()
{
    if (!m_containmentActionConfigModel) {
        m_containmentActionConfigModel = new ConfigModel(this);

        KPluginInfo::List actions = Plasma::PluginLoader::self()->listContainmentActionsInfo(QString());

        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");

        foreach (const KPluginInfo &info, actions) {
            pkg.setDefaultPackageRoot(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "plasma/containmentactions", QStandardPaths::LocateDirectory));
            ConfigCategory *cat = new ConfigCategory(m_containmentActionConfigModel);
            cat->setName(info.name());
            cat->setIcon(info.icon());
            cat->setSource(pkg.filePath("ui", "config.qml"));
            cat->setPluginName(info.pluginName());
            m_containmentActionConfigModel->appendCategory(cat);
        }

    }
    return m_containmentActionConfigModel;
}

QStandardItemModel *ContainmentConfigView::currentContainmentActionsModel()
{
    if (!m_currentContainmentActionsModel) {
        m_currentContainmentActionsModel = new CurrentContainmentActionsModel(m_containment, this);
    }
    return m_currentContainmentActionsModel;
}

ConfigModel *ContainmentConfigView::wallpaperConfigModel()
{
    if (!m_wallpaperConfigModel) {
        m_wallpaperConfigModel = new ConfigModel(this);
        QStringList dirs(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "plasma/wallpapers", QStandardPaths::LocateDirectory));
        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
        foreach (const QString &dirPath, dirs) {
            QDir dir(dirPath);
            pkg.setDefaultPackageRoot(dirPath);
            QStringList packages;

            foreach (const QString &sdir, dir.entryList(QDir::AllDirs | QDir::Readable)) {
                QString metadata = dirPath + '/' + sdir + "/metadata.desktop";
                if (QFile::exists(metadata)) {
                    packages << sdir;
                }
            }

            foreach (const QString &package, packages) {
                pkg.setPath(package);
                if (!pkg.isValid()) {
                    continue;
                }
                ConfigCategory *cat = new ConfigCategory(m_wallpaperConfigModel);
                cat->setName(pkg.metadata().name());
                cat->setIcon(pkg.metadata().icon());
                cat->setSource(pkg.filePath("ui", "config.qml"));
                cat->setPluginName(package);
                m_wallpaperConfigModel->appendCategory(cat);
            }
        }
    }
    return m_wallpaperConfigModel;
}

ConfigPropertyMap *ContainmentConfigView::wallpaperConfiguration() const
{
    return m_currentWallpaperConfig;
}

QString ContainmentConfigView::currentWallpaper() const
{
    return m_currentWallpaper;
}

void ContainmentConfigView::setCurrentWallpaper(const QString &wallpaper)
{
    if (m_currentWallpaper == wallpaper) {
        return;
    }

    delete m_ownWallpaperConfig;
    m_ownWallpaperConfig = 0;

    if (m_containment->wallpaper() == wallpaper) {
        syncWallpaperObjects();
    } else {

        //we have to construct an independent ConfigPropertyMap when we want to configure wallpapers that are not the current one
        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
        pkg.setDefaultPackageRoot("plasma/wallpapers");
        pkg.setPath(wallpaper);
        QFile file(pkg.filePath("config", "main.xml"));
        KConfigGroup cfg = m_containment->config();
        cfg = KConfigGroup(&cfg, "Wallpaper");
        m_currentWallpaperConfig = m_ownWallpaperConfig = new ConfigPropertyMap(new Plasma::ConfigLoader(&cfg, &file), this);
    }

    m_currentWallpaper = wallpaper;
    emit currentWallpaperChanged();
    emit wallpaperConfigurationChanged();
}

void ContainmentConfigView::applyWallpaper()
{
    m_containment->setWallpaper(m_currentWallpaper);

    delete m_ownWallpaperConfig;
    m_ownWallpaperConfig = 0;

    syncWallpaperObjects();
    emit wallpaperConfigurationChanged();
}

void ContainmentConfigView::syncWallpaperObjects()
{
    QObject *wallpaperGraphicsObject = m_containment->property("wallpaperGraphicsObject").value<QObject *>();
    engine()->rootContext()->setContextProperty("wallpaper", wallpaperGraphicsObject);

    //FIXME: why m_wallpaperGraphicsObject->property("configuration").value<ConfigPropertyMap *>() doesn't work?
    m_currentWallpaperConfig = static_cast<ConfigPropertyMap *>(wallpaperGraphicsObject->property("configuration").value<QObject *>());
}

#include "moc_containmentconfigview.cpp"
