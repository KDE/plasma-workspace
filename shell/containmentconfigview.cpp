/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "containmentconfigview.h"
#include "config-workspace.h"
#include "currentcontainmentactionsmodel.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>

#include <KConfigLoader>
#include <KConfigPropertyMap>
#include <KLocalizedString>
#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <Plasma/ContainmentActions>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

class WallpaperConfigModel : public PlasmaQuick::ConfigModel
{
    Q_OBJECT
public:
    WallpaperConfigModel(QObject *parent);
public Q_SLOTS:
    void repopulate();
};

//////////////////////////////ContainmentConfigView
ContainmentConfigView::ContainmentConfigView(Plasma::Containment *cont, QWindow *parent)
    : ConfigView(cont, parent)
    , m_containment(cont)
{
    qmlRegisterAnonymousType<QAbstractItemModel>("QAbstractItemModel", 1);
    setCurrentWallpaper(cont->containment()->wallpaperPlugin());

    KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    pkg.setPath(m_containment->wallpaperPlugin());
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

        const QList<KPluginMetaData> actions = Plasma::PluginLoader::self()->listContainmentActionsMetaData(QString());

        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Generic"));

        for (const KPluginMetaData &plugin : actions) {
            pkg.setDefaultPackageRoot(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                             QStringLiteral(PLASMA_RELATIVE_DATA_INSTALL_DIR "/containmentactions"),
                                                             QStandardPaths::LocateDirectory));
            m_containmentActionConfigModel->appendCategory(plugin.iconName(),
                                                           plugin.name(),
                                                           pkg.filePath("ui", QStringLiteral("config.qml")),
                                                           plugin.pluginId());
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
    Q_EMIT containmentPluginChanged();
}

PlasmaQuick::ConfigModel *ContainmentConfigView::wallpaperConfigModel()
{
    if (!m_wallpaperConfigModel) {
        m_wallpaperConfigModel = new WallpaperConfigModel(this);
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KPackage/Plasma/Wallpaper"),
                                              QStringLiteral("org.kde.plasma.kpackage"),
                                              QStringLiteral("packageInstalled"),
                                              m_wallpaperConfigModel,
                                              SLOT(repopulate()));
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KPackage/Plasma/Wallpaper"),
                                              QStringLiteral("org.kde.plasma.kpackage"),
                                              QStringLiteral("packageUpdated"),
                                              m_wallpaperConfigModel,
                                              SLOT(repopulate()));
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KPackage/Plasma/Wallpaper"),
                                              QStringLiteral("org.kde.plasma.kpackage"),
                                              QStringLiteral("packageUninstalled"),
                                              m_wallpaperConfigModel,
                                              SLOT(repopulate()));
    }
    return m_wallpaperConfigModel;
}

PlasmaQuick::ConfigModel *ContainmentConfigView::containmentPluginsConfigModel()
{
    if (!m_containmentPluginsConfigModel) {
        m_containmentPluginsConfigModel = new PlasmaQuick::ConfigModel(this);

        const QList<KPluginMetaData> actions = Plasma::PluginLoader::self()->listContainmentsMetaDataOfType(QStringLiteral("Desktop"));
        for (const KPluginMetaData &plugin : actions) {
            m_containmentPluginsConfigModel->appendCategory(plugin.iconName(), plugin.name(), QString(), plugin.pluginId());
        }
    }
    return m_containmentPluginsConfigModel;
}

QQmlPropertyMap *ContainmentConfigView::wallpaperConfiguration() const
{
    return m_currentWallpaperConfig;
}

QString ContainmentConfigView::currentWallpaper() const
{
    return m_currentWallpaperPlugin;
}

void ContainmentConfigView::setCurrentWallpaper(const QString &wallpaperPlugin)
{
    if (m_currentWallpaperPlugin == wallpaperPlugin) {
        return;
    }

    delete m_ownWallpaperConfig;
    m_ownWallpaperConfig = nullptr;

    if (m_containment->wallpaperPlugin() == wallpaperPlugin) {
        syncWallpaperObjects();
    } else {
        // we have to construct an independent ConfigPropertyMap when we want to configure wallpapers that are not the current one
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Generic"));
        pkg.setDefaultPackageRoot(QStringLiteral(PLASMA_RELATIVE_DATA_INSTALL_DIR "/wallpapers"));
        pkg.setPath(wallpaperPlugin);
        QFile file(pkg.filePath("config", QStringLiteral("main.xml")));
        KConfigGroup cfg = m_containment->config().group("Wallpaper").group(wallpaperPlugin);
        m_currentWallpaperConfig = m_ownWallpaperConfig = new KConfigPropertyMap(new KConfigLoader(cfg, &file, this), this);
    }

    m_currentWallpaperPlugin = wallpaperPlugin;
    Q_EMIT currentWallpaperChanged();
    Q_EMIT wallpaperConfigurationChanged();
}

void ContainmentConfigView::applyWallpaper()
{
    static_cast<KConfigPropertyMap *>(m_currentWallpaperConfig)->writeConfig();
    m_containment->setWallpaperPlugin(m_currentWallpaperPlugin);
    syncWallpaperObjects();

    Q_EMIT wallpaperConfigurationChanged();
}

void ContainmentConfigView::syncWallpaperObjects()
{
    QObject *wallpaperGraphicsObject = m_containment->property("wallpaperGraphicsObject").value<QObject *>();

    if (!wallpaperGraphicsObject) {
        return;
    }
    rootContext()->setContextProperty(QStringLiteral("wallpaper"), wallpaperGraphicsObject);

    // FIXME: why m_wallpaperGraphicsObject->property("configuration").value<ConfigPropertyMap *>() doesn't work?
    m_currentWallpaperConfig = static_cast<QQmlPropertyMap *>(wallpaperGraphicsObject->property("configuration").value<QObject *>());
}

WallpaperConfigModel::WallpaperConfigModel(QObject *parent)
    : PlasmaQuick::ConfigModel(parent)
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
