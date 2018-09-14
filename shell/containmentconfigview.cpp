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
#include "plasmaquick/configmodel.h"
#include "shellcorona.h"
#include "config-workspace.h"

#include <KDeclarative/KDeclarative>

#include <kdeclarative/configpropertymap.h>
#include <kconfigloader.h>

#include <QDebug>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDBusConnection>

#include <klocalizedstring.h>

#include <Plasma/Corona>
#include <Plasma/PluginLoader>
#include <Plasma/ContainmentActions>
#include <qstandardpaths.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

class WallpaperConfigModel: public PlasmaQuick::ConfigModel
{
    Q_OBJECT
public:
    WallpaperConfigModel(QObject *parent);
public Q_SLOTS:
    void repopulate();
};


//////////////////////////////ContainmentConfigView
ContainmentConfigView::ContainmentConfigView(Plasma::Containment *cont, QWindow *parent)
    : ConfigView(cont, parent),
      m_containment(cont)
{
    qmlRegisterType<QAbstractItemModel>();
    rootContext()->setContextProperty(QStringLiteral("configDialog"), this);
    setCurrentWallpaper(cont->containment()->wallpaper());

    KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    pkg.setPath(m_containment->wallpaper());
    KConfigGroup cfg = m_containment->config();
    cfg = KConfigGroup(&cfg, "Wallpaper");

    syncWallpaperObjects();
}

ContainmentConfigView::~ContainmentConfigView()
{
}

void ContainmentConfigView::init()
{
    setSource(m_containment->corona()->kPackage().fileUrl("containmentconfigurationui"));
}

PlasmaQuick::ConfigModel *ContainmentConfigView::containmentActionConfigModel()
{
    if (!m_containmentActionConfigModel) {
        m_containmentActionConfigModel = new PlasmaQuick::ConfigModel(this);

        KPluginInfo::List actions = Plasma::PluginLoader::self()->listContainmentActionsInfo(QString());

        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Generic"));

        foreach (const KPluginInfo &info, actions) {
            pkg.setDefaultPackageRoot(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral(PLASMA_RELATIVE_DATA_INSTALL_DIR "/containmentactions"), QStandardPaths::LocateDirectory));
            m_containmentActionConfigModel->appendCategory(info.icon(), info.name(), pkg.filePath("ui", QStringLiteral("config.qml")), info.pluginName());
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
    return m_containment->pluginMetaData().pluginId();
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
        m_wallpaperConfigModel = new WallpaperConfigModel(this);
        QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/KPackage/Plasma/Wallpaper"), QStringLiteral("org.kde.plasma.kpackage"), QStringLiteral("packageInstalled"),
            m_wallpaperConfigModel, SLOT(repopulate())) ;
        QDBusConnection::sessionBus().connect(QString(), QStringLiteral("/KPackage/Plasma/Wallpaper"), QStringLiteral("org.kde.plasma.kpackage"), QStringLiteral("packageUninstalled"),
            m_wallpaperConfigModel, SLOT(repopulate()));

    }
    return m_wallpaperConfigModel;
}

PlasmaQuick::ConfigModel *ContainmentConfigView::containmentPluginsConfigModel()
{
    if (!m_containmentPluginsConfigModel) {
        m_containmentPluginsConfigModel = new PlasmaQuick::ConfigModel(this);

        KPluginInfo::List actions = Plasma::PluginLoader::self()->listContainmentsOfType(QStringLiteral("Desktop"));

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
    m_ownWallpaperConfig = nullptr;

    if (m_containment->wallpaper() == wallpaper) {
        syncWallpaperObjects();
    } else {

        //we have to construct an independent ConfigPropertyMap when we want to configure wallpapers that are not the current one
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Generic"));
        pkg.setDefaultPackageRoot(QStringLiteral(PLASMA_RELATIVE_DATA_INSTALL_DIR "/wallpapers"));
        pkg.setPath(wallpaper);
        QFile file(pkg.filePath("config", QStringLiteral("main.xml")));
        KConfigGroup cfg = m_containment->config();
        cfg = KConfigGroup(&cfg, "Wallpaper");
        cfg = KConfigGroup(&cfg, wallpaper);
        m_currentWallpaperConfig = m_ownWallpaperConfig = new KDeclarative::ConfigPropertyMap(new KConfigLoader(cfg, &file), this);
    }

    m_currentWallpaper = wallpaper;
    emit currentWallpaperChanged();
    emit wallpaperConfigurationChanged();
}

void ContainmentConfigView::applyWallpaper()
{
    m_containment->setWallpaper(m_currentWallpaper);

    syncWallpaperObjects();

    if (m_currentWallpaperConfig && m_ownWallpaperConfig) {
        for (const auto &key : m_ownWallpaperConfig->keys()) {
            auto value = m_ownWallpaperConfig->value(key);
            m_currentWallpaperConfig->insert(key, value);
            m_currentWallpaperConfig->valueChanged(key, value);
        }
    }

    delete m_ownWallpaperConfig;
    m_ownWallpaperConfig = nullptr;

    emit wallpaperConfigurationChanged();
}

void ContainmentConfigView::syncWallpaperObjects()
{
    QObject *wallpaperGraphicsObject = m_containment->property("wallpaperGraphicsObject").value<QObject *>();

    if (!wallpaperGraphicsObject) {
        return;
    }
    rootContext()->setContextProperty(QStringLiteral("wallpaper"), wallpaperGraphicsObject);

    //FIXME: why m_wallpaperGraphicsObject->property("configuration").value<ConfigPropertyMap *>() doesn't work?
    m_currentWallpaperConfig = static_cast<KDeclarative::ConfigPropertyMap *>(wallpaperGraphicsObject->property("configuration").value<QObject *>());
}

WallpaperConfigModel::WallpaperConfigModel(QObject *parent)
    :PlasmaQuick::ConfigModel(parent)
{
    repopulate();
}

void WallpaperConfigModel::repopulate()
{
    clear();
    for (const KPluginMetaData &m : KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/Wallpaper"))) {
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"), m.pluginId());
        if (!pkg.isValid()) {
            continue;
        }
        appendCategory(pkg.metadata().iconName(), pkg.metadata().name(), pkg.fileUrl("ui", QStringLiteral("config.qml")).toString(), m.pluginId());
    }
}

#include "containmentconfigview.moc"
