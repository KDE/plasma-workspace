#ifndef WALLPAPERMODULE_H
#define WALLPAPERMODULE_H

#include <KConfigGroup>
#include <KQuickConfigModule>
#include <KSharedConfig>
#include <PlasmaQuick/ConfigModel>

#include <QQmlPropertyMap>
#include <QScreen>

class KConfigLoader;
class WallpaperConfigModel;
class OutputOrderWatcher;
class KConfigPropertyMap;

namespace KActivities
{
class Consumer;
}

class WallpaperModule : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(QQmlPropertyMap *configuration READ wallpaperConfiguration NOTIFY wallpaperConfigurationChanged)

    Q_PROPERTY(QString currentWallpaper READ currentWallpaperPlugin WRITE setCurrentWallpaperPlugin NOTIFY currentWallpaperPluginChanged)
    Q_PROPERTY(QString wallpaperPluginSource READ wallpaperPluginSource NOTIFY currentWallpaperPluginChanged)
    Q_PROPERTY(PlasmaQuick::ConfigModel *wallpaperConfigModel READ wallpaperConfigModel CONSTANT)

    Q_PROPERTY(QScreen *selectedScreen READ selectedScreen NOTIFY selectedScreenChanged)
    Q_PROPERTY(QList<QScreen *> screens READ screens NOTIFY screensChanged)

public:
    explicit WallpaperModule(QObject *parent, const KPluginMetaData &data);

    PlasmaQuick::ConfigModel *wallpaperConfigModel();

    QQmlPropertyMap *wallpaperConfiguration() const;
    QString wallpaperPluginSource();
    QList<QScreen *> screens() const;
    QScreen *selectedScreen() const;

    Q_INVOKABLE void setSelectedScreen(const QString &screenName);
    int screenIdFromName(const QString &stringName) const;

    QString currentWallpaperPlugin() const;
    void setCurrentWallpaperPlugin(const QString &wallpaperPlugin);

    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void wallpaperConfigurationChanged();
    void currentWallpaperPluginChanged();
    void selectedScreenChanged();
    void screensChanged();
    void settingsSaved();

public Q_SLOTS:
    void onScreenChanged();
    void onWallpaperChanged(uint screenIdx);
    void setCurrentActivity(const QString &activityId);

private:
    void setWallpaperPluginConfiguration(const QString &wallpaperplugin, bool loadDefaults = false);
    void loadConfiguration();
    bool isDefault() const;

    KSharedConfig::Ptr m_config;
    KConfigLoader *m_configLoader;
    KActivities::Consumer *m_activityConsumer = nullptr;
    OutputOrderWatcher *m_outputOrderWatcher = nullptr;
    WallpaperConfigModel *m_wallpaperConfigModel = nullptr;
    KConfigPropertyMap *m_wallpaperConfiguration = nullptr;
    QString m_loadedWallpaperplugin;
    QString m_currentWallpaperPlugin;
    QScreen *m_selectedScreen = nullptr;
    KConfigGroup m_wallpaperConfigGeneral;
    QString m_activityId;
    QString m_containmentIdx;
    QString m_defaultWallpaper;
    QList<QScreen *> m_screens;
};

#endif // WALLPAPERMODULE_H
