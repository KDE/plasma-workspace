#include "wallpapermodule.h"

#include "config-workspace.h"
#include "defaultwallpaper.h"
#include "kcm_wallpaper_debug.h"
#include "qdbusinterface.h"
#include "qdbusreply.h"
#include <defaultwallpaper.h>
#include <outputorderwatcher.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

#include <KConfig>
#include <KConfigGroup>
#include <KConfigLoader>
#include <KLocalizedString>
#include <KPluginFactory>
#include <plasmaactivities/consumer.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QFile>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QWindow>

K_PLUGIN_CLASS_WITH_JSON(WallpaperModule, "kcm_wallpaper.json")

Q_DECLARE_METATYPE(QColor)

using namespace Qt::StringLiterals;

QDBusArgument &operator<<(QDBusArgument &argument, const QColor &color)
{
    argument.beginStructure();
    argument << color.rgba();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QColor &color)
{
    argument.beginStructure();
    QRgb rgba;
    argument >> rgba;
    argument.endStructure();
    color = QColor::fromRgba(rgba);
    return argument;
}

WallpaperModule::~WallpaperModule()
{
    disconnect(m_wallpaperConfiguration.get(), &QQmlPropertyMap::valueChanged, this, nullptr);
}

WallpaperModule::WallpaperModule(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
    , m_config(KSharedConfig::openConfig(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/plasma-org.kde.plasma.desktop-appletsrc",
                                         KConfig::OpenFlag::SimpleConfig))
    , m_activityConsumer(new KActivities::Consumer(this))
{
    qDBusRegisterMetaType<QColor>();

    KActivities::Consumer c(this);

    connect(m_activityConsumer, &KActivities::Consumer::currentActivityChanged, this, &WallpaperModule::setCurrentActivity);

    constexpr const char *uri = "org.kde.plasma.kcm.wallpaper";

    qmlRegisterAnonymousType<QScreen>(uri, 1);
    qmlRegisterAnonymousType<KConfigPropertyMap>(uri, 1);

    m_outputOrderWatcher = OutputOrderWatcher::instance(this);
    connect(m_outputOrderWatcher, &OutputOrderWatcher::outputOrderChanged, this, [this](const QStringList &outputOrder) {
        if (!m_selectedScreen) {
            return;
        }
        if (!outputOrder.contains(m_selectedScreen->name())) {
            // current screen was removed
            m_selectedScreen = mainUi()->window()->screen();
            Q_EMIT selectedScreenChanged();
        }
        onScreenChanged();
    });

    connect(this, &KQuickConfigModule::mainUiReady, this, [this]() {
        connect(
            mainUi(),
            &QQuickItem::windowChanged,
            this,
            [this]() {
                m_selectedScreen = mainUi()->window()->screen();
                Q_EMIT selectedScreenChanged();
                onScreenChanged();
            },
            Qt::ConnectionType::SingleShotConnection);
    });

    connectToPlasmaShell();

    setButtons(Apply | Default);

    m_screens = qApp->screens();
    connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
        m_screens.removeAll(screen);
        Q_EMIT screensChanged();
    });
    connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        m_screens << screen;
        Q_EMIT screensChanged();
    });
}

void WallpaperModule::connectToPlasmaShell()
{
    const bool connected = QDBusConnection::sessionBus().connect(QStringLiteral("org.kde.plasmashell"),
                                                                 QStringLiteral("/PlasmaShell"),
                                                                 QStringLiteral("org.kde.PlasmaShell"),
                                                                 QStringLiteral("wallpaperChanged"),
                                                                 this,
                                                                 SLOT(onWallpaperChanged(uint)));
    if (!connected) {
        qCFatal(KCM_WALLPAPER_DEBUG) << "Could not connect to dbus service org.kde.plasmashell";
    }
}

void WallpaperModule::disconnectFromPlasmaShell()
{
    QDBusConnection::sessionBus().disconnect(QStringLiteral("org.kde.plasmashell"),
                                             QStringLiteral("/PlasmaShell"),
                                             QStringLiteral("org.kde.PlasmaShell"),
                                             QStringLiteral("wallpaperChanged"),
                                             this,
                                             SLOT(onWallpaperChanged(uint)));
}

void WallpaperModule::onScreenChanged()
{
    if (m_activityId.isEmpty() || m_activityId == QUuid().toString()) {
        return;
    }

    if (m_selectedScreen == nullptr) {
        return;
    }

    const auto outputOrder = m_outputOrderWatcher->outputOrder();
    if (outputOrder.isEmpty()) {
        return;
    }
    int screenId = screenIdFromName(m_selectedScreen->name());
    Q_ASSERT(screenId != -1);

    auto containmentsGroup = m_config->group(QStringLiteral("Containments"));

    for (const auto &contIndex : containmentsGroup.groupList()) {
        const auto contConfig = containmentsGroup.group(contIndex);
        if (m_activityId != contConfig.readEntry("activityId")) {
            continue;
        }

        auto lastScreenIdx = contConfig.readEntry("lastScreen", -1);
        if (lastScreenIdx < 0) {
            continue;
        }

        if (lastScreenIdx == screenId) {
            m_containmentIdx = contIndex;
            break;
        }
    }
    Q_ASSERT(!m_containmentIdx.isEmpty());

    auto containmentConfigGroup = containmentsGroup.group(m_containmentIdx);
    m_loadedWallpaperplugin = containmentConfigGroup.readEntry("wallpaperplugin");

    setWallpaperPluginConfiguration(m_loadedWallpaperplugin);

    setRepresentsDefaults(isDefault());
    setNeedsSave(false);

    if (m_loadedWallpaperplugin != m_currentWallpaperPlugin) {
        m_currentWallpaperPlugin = m_loadedWallpaperplugin;
        Q_EMIT currentWallpaperPluginChanged();
    } else {
        Q_EMIT wallpaperConfigurationChanged();
    }
}

int WallpaperModule::screenIdFromName(const QString &screenName) const
{
    int screenId = -1;
    int idx = 0;
    const auto outputOrder = m_outputOrderWatcher->outputOrder();
    for (const auto &output : outputOrder) {
        if (output == screenName) {
            screenId = idx;
            break;
        }
        ++idx;
    }

    return screenId;
}

void WallpaperModule::setWallpaperPluginConfiguration(const QString &wallpaperplugin, bool loadDefaults)
{
    // avoid recursive updating state
    disconnectFromPlasmaShell();

    auto wallpaperConfig = m_config->group(QStringLiteral("Containments")).group(m_containmentIdx).group(QStringLiteral("Wallpaper")).group(wallpaperplugin);
    m_wallpaperConfigGeneral = wallpaperConfig.group(QStringLiteral("General"));

    KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Wallpaper"));
    pkg.setDefaultPackageRoot(QStringLiteral(PLASMA_RELATIVE_DATA_INSTALL_DIR "/wallpapers"));
    pkg.setPath(wallpaperplugin);
    QFile file(pkg.filePath("config", QStringLiteral("main.xml")));

    m_configLoader = new KConfigLoader(wallpaperConfig, &file, this);
    if (loadDefaults) {
        m_configLoader->setDefaults();
    }
    m_configLoader->removeItem(QStringLiteral("PreviewImage"));
    m_wallpaperConfiguration.reset(new KConfigPropertyMap(m_configLoader, this));

    // set the default wallpaper value
    m_defaultWallpaper = DefaultWallpaper::defaultWallpaperPackage().path();
    m_wallpaperConfiguration->insert(QStringLiteral("ImageDefault"), m_defaultWallpaper);

    // set the default Image value if necessary
    if (m_wallpaperConfiguration->value(QStringLiteral("Image")).isNull()) {
        m_wallpaperConfiguration->insert(QStringLiteral("Image"), m_defaultWallpaper);
    }

    connect(m_wallpaperConfiguration.get(), &QQmlPropertyMap::valueChanged, this, [this](const QString & /* key */, const QVariant & /* value */) {
        setRepresentsDefaults(isDefault());
        setNeedsSave(m_configLoader->isSaveNeeded() || m_loadedWallpaperplugin != m_currentWallpaperPlugin);
    });

    connectToPlasmaShell();
}

class WallpaperConfigModel : public PlasmaQuick::ConfigModel
{
    Q_OBJECT

public:
    WallpaperConfigModel(QObject *parent);
public Q_SLOTS:
    void repopulate();
Q_SIGNALS:
    void wallpaperPluginsChanged();
};

void WallpaperModule::onWallpaperChanged(uint screenIdx)
{
    m_config->markAsClean();
    m_config->reparseConfiguration();

    Q_ASSERT(m_selectedScreen);
    int screen = screenIdFromName(m_selectedScreen->name());
    if (screen >= 0 && screenIdx == uint(screen)) {
        onScreenChanged();
    }
}

void WallpaperModule::setCurrentActivity(const QString &activityId)
{
    if (m_activityId != activityId) {
        m_activityId = activityId;
        onScreenChanged();
    }
}

bool WallpaperModule::isDefault() const
{
    if (m_currentWallpaperPlugin != QStringLiteral("org.kde.image")) {
        return false;
    }
    for (const auto &item : m_configLoader->items()) {
        if (!item->isDefault()) {
            if (item->name() == u"Image" && item->property() == m_defaultWallpaper) {
                continue;
            }
            if (item->name() == u"SlidePaths") {
                continue;
            }
            return false;
        }
    }
    return true;
}

void WallpaperModule::defaults()
{
    KQuickConfigModule::defaults();

    if (m_currentWallpaperPlugin != QStringLiteral("org.kde.image")) {
        setCurrentWallpaperPlugin(QStringLiteral("org.kde.image"));
        Q_EMIT currentWallpaperPluginChanged();
    }

    setWallpaperPluginConfiguration(m_currentWallpaperPlugin, true);
    m_wallpaperConfiguration->insert(QStringLiteral("Image"), m_defaultWallpaper);

    setRepresentsDefaults(isDefault());
    setNeedsSave(m_configLoader->isSaveNeeded() || m_loadedWallpaperplugin != m_currentWallpaperPlugin);

    Q_EMIT wallpaperConfigurationChanged();
}

void WallpaperModule::load()
{
    KQuickConfigModule::load();
    onScreenChanged();
}

void WallpaperModule::save()
{
    KQuickConfigModule::save();
    auto iface = new QDBusInterface(u"org.kde.plasmashell"_s, u"/PlasmaShell"_s, u"org.kde.PlasmaShell"_s, QDBusConnection::sessionBus(), this);
    if (!iface->isValid()) {
        qCWarning(KCM_WALLPAPER_DEBUG) << qPrintable(QDBusConnection::sessionBus().lastError().message());
        return;
    }

    QList<uint> screenIds;

    if (m_allScreens) {
        const auto outputOrder = m_outputOrderWatcher->outputOrder();
        for (uint idx = 0; idx < outputOrder.size(); ++idx) {
            screenIds.append(idx);
        }
    } else {
        int screenId = screenIdFromName(m_selectedScreen->name());
        if (screenId == -1) {
            qCWarning(KCM_WALLPAPER_DEBUG) << "Screen not found";
            return;
        }
        screenIds.append(uint(screenId));
    }

    QVariantMap params;
    for (const auto &key : m_wallpaperConfiguration->keys()) {
        const QVariant value = m_wallpaperConfiguration->value(key);
        if (value.isValid()) { // https://bugreports.qt.io/browse/QTBUG-130038
            params.insert(key, value);
        }
    }

    if (m_currentWallpaperPlugin == QLatin1String("org.kde.image")) {
        params.remove(QStringLiteral("PreviewImage"));
    }

    for (const uint screenId : screenIds) {
        const QDBusReply<void> response = iface->call(QStringLiteral("setWallpaper"), m_currentWallpaperPlugin, params, screenId);
        if (!response.isValid()) {
            qCWarning(KCM_WALLPAPER_DEBUG) << "failed to set wallpaper:" << response.error();
        }
    }

    setRepresentsDefaults(isDefault());
    Q_EMIT settingsSaved();
}

QQmlPropertyMap *WallpaperModule::wallpaperConfiguration() const
{
    return m_wallpaperConfiguration.get();
}

QString WallpaperModule::currentWallpaperPlugin() const
{
    return m_currentWallpaperPlugin;
}

bool WallpaperModule::allScreens() const
{
    return m_allScreens;
}

void WallpaperModule::setAllScreens(const bool allScreens)
{
    if (allScreens != m_allScreens) {
        m_allScreens = allScreens;
        setNeedsSave(m_allScreens || m_configLoader->isSaveNeeded() || m_loadedWallpaperplugin != m_currentWallpaperPlugin);
        Q_EMIT allScreensChanged();
    }
}

QList<QScreen *> WallpaperModule::screens() const
{
    return m_screens;
}

QScreen *WallpaperModule::selectedScreen() const
{
    return m_selectedScreen;
}

void WallpaperModule::setSelectedScreen(const QString &screenName)
{
    const auto screens = qApp->screens();
    auto it = std::find_if(screens.constBegin(), screens.cend(), [screenName](const QScreen *screen) {
        return screen->name() == screenName;
    });
    if (it != screens.cend() && m_selectedScreen != *it) {
        m_selectedScreen = *it;
        Q_EMIT selectedScreenChanged();

        onScreenChanged();
    }
}

QString WallpaperModule::wallpaperPluginSource()
{
    if (m_currentWallpaperPlugin.isEmpty()) {
        return QString();
    }

    const auto model = wallpaperConfigModel();
    const auto wallpaperPluginCount = model->count();
    for (int i = 0; i < wallpaperPluginCount; ++i) {
        if (model->data(model->index(i), PlasmaQuick::ConfigModel::PluginNameRole) == m_currentWallpaperPlugin) {
            return model->data(model->index(i), PlasmaQuick::ConfigModel::SourceRole).toString();
        }
    }

    return QString();
}

void WallpaperModule::setCurrentWallpaperPlugin(const QString &wallpaperPlugin)
{
    if (wallpaperPlugin == m_currentWallpaperPlugin) {
        return;
    }

    m_currentWallpaperPlugin = wallpaperPlugin;

    setWallpaperPluginConfiguration(m_currentWallpaperPlugin);

    setNeedsSave(needsSave() || m_loadedWallpaperplugin != m_currentWallpaperPlugin);

    Q_EMIT currentWallpaperPluginChanged();
}

PlasmaQuick::ConfigModel *WallpaperModule::wallpaperConfigModel()
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
    Q_EMIT wallpaperPluginsChanged();
}

#include "wallpapermodule.moc"

#include "moc_wallpapermodule.cpp"
