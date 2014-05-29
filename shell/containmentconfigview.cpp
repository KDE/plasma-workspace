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

#include "currentcontainmentactionsmodel.h"
#include "containmentconfigview.h"
#include "configmodel.h"
#include "shellcorona.h"
#include "config-workspace.h"

#include <kdeclarative/configpropertymap.h>
#include <kconfigloader.h>

#include <QDebug>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>

#include <klocalizedstring.h>

#include <Plasma/Corona>
#include <Plasma/ContainmentActions>
#include <Plasma/PluginLoader>
#include <qstandardpaths.h>

//////////////////////////////ContainmentConfigView
ContainmentConfigView::ContainmentConfigView(Plasma::Containment *cont, QWindow *parent)
    : ConfigView(cont, parent),
      m_containment(cont),
      m_wallpaperConfigModel(0),
      m_containmentActionConfigModel(0),
      m_currentContainmentActionsModel(0),
      m_containmentPluginsConfigModel(0),
      m_currentWallpaperConfig(0),
      m_ownWallpaperConfig(0)
{
    qmlRegisterType<QAbstractItemModel>();
    engine()->rootContext()->setContextProperty("configDialog", this);
    setCurrentWallpaper(cont->containment()->wallpaper());

    Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Wallpaper");
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
    setSource(QUrl::fromLocalFile(m_containment->corona()->package().filePath("containmentconfigurationui")));
}

PlasmaQuick::ConfigModel *ContainmentConfigView::containmentActionConfigModel()
{
    if (!m_containmentActionConfigModel) {
        m_containmentActionConfigModel = new PlasmaQuick::ConfigModel(this);

        KPluginInfo::List actions = Plasma::PluginLoader::self()->listContainmentActionsInfo(QString());

        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");

        foreach (const KPluginInfo &info, actions) {
            pkg.setDefaultPackageRoot(QStandardPaths::locate(QStandardPaths::GenericDataLocation, PLASMA_RELATIVE_DATA_INSTALL_DIR "/containmentactions", QStandardPaths::LocateDirectory));
            m_containmentActionConfigModel->appendCategory(info.icon(), info.name(), pkg.filePath("ui", "config.qml"), info.pluginName());
        }

    }
    return m_containmentActionConfigModel;
}

QAbstractItemModel *ContainmentConfigView::currentContainmentActionsModel()
{
    if (!m_currentContainmentActionsModel) {
        m_currentContainmentActionsModel = new CurrentContainmentActionsModel(m_containment, this);
    }
    return m_currentContainmentActionsModel;
}

QString ContainmentConfigView::containmentPlugin() const
{
    return m_containment->pluginInfo().pluginName();
}

void ContainmentConfigView::setContainmentPlugin(const QString &plugin)
{
    if (plugin.isEmpty() || containmentPlugin() == plugin) {
        return;
    }

    m_containment = static_cast<ShellCorona *>(m_containment->corona())->setContainmentTypeForScreen(m_containment->screen(), plugin);
    emit containmentPluginChanged();
}

PlasmaQuick::ConfigModel *ContainmentConfigView::wallpaperConfigModel()
{
    if (!m_wallpaperConfigModel) {
        m_wallpaperConfigModel = new PlasmaQuick::ConfigModel(this);
        QStringList dirs(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, PLASMA_RELATIVE_DATA_INSTALL_DIR "/wallpapers", QStandardPaths::LocateDirectory));
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
                m_wallpaperConfigModel->appendCategory(pkg.metadata().icon(), pkg.metadata().name(), pkg.filePath("ui", "config.qml"), package);
            }
        }
    }
    return m_wallpaperConfigModel;
}

PlasmaQuick::ConfigModel *ContainmentConfigView::containmentPluginsConfigModel()
{
    if (!m_containmentPluginsConfigModel) {
        m_containmentPluginsConfigModel = new PlasmaQuick::ConfigModel(this);

        KPluginInfo::List actions = Plasma::PluginLoader::self()->listContainmentsOfType("Desktop");

        foreach (const KPluginInfo &info, actions) {
            m_containmentPluginsConfigModel->appendCategory(info.icon(), info.name(), QString(), info.pluginName());
        }

    }
    return m_containmentPluginsConfigModel;
}

KDeclarative::ConfigPropertyMap *ContainmentConfigView::wallpaperConfiguration() const
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
        pkg.setDefaultPackageRoot(PLASMA_RELATIVE_DATA_INSTALL_DIR "/wallpapers");
        pkg.setPath(wallpaper);
        QFile file(pkg.filePath("config", "main.xml"));
        KConfigGroup cfg = m_containment->config();
        cfg = KConfigGroup(&cfg, "Wallpaper");
        m_currentWallpaperConfig = m_ownWallpaperConfig = new KDeclarative::ConfigPropertyMap(new KConfigLoader(cfg, &file), this);
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

    if (!wallpaperGraphicsObject) {
        return;
    }
    engine()->rootContext()->setContextProperty("wallpaper", wallpaperGraphicsObject);

    //FIXME: why m_wallpaperGraphicsObject->property("configuration").value<ConfigPropertyMap *>() doesn't work?
    m_currentWallpaperConfig = static_cast<KDeclarative::ConfigPropertyMap *>(wallpaperGraphicsObject->property("configuration").value<QObject *>());
}

#include "private/moc_containmentconfigview.cpp"
