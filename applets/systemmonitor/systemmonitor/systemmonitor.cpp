/***************************************************************************
 *   Copyright (C) 2019 Marco Martin <mart@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "systemmonitor.h"

#include <QtQml>
#include <QDebug>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QWindow>

#include <sensors/SensorQuery.h>
#include <sensors/SensorFace.h>
#include <sensors/SensorFaceController.h>

#include <KConfigLoader>
#include <KLocalizedString>
#include <KPackage/PackageLoader>
#include <KDeclarative/ConfigPropertyMap>
#include <KDeclarative/QmlObjectSharedEngine>

#include <KNewPasswordDialog>

FacesModel::FacesModel(QObject *parent)
    : QStandardItemModel(parent)
{}

void FacesModel::load()
{
    clear();

    auto list = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/SensorApplet"));
    // NOTE: This will diable completely the internal in-memory cache 
    KPackage::Package p;
    p.install(QString(), QString());

    for (auto plugin : list) {
        QStandardItem *item = new QStandardItem(plugin.name());
        item->setData(plugin.name(), FacesModel::ModelDataRole);
        item->setData(plugin.pluginId(), FacesModel::PluginIdRole);
        appendRow(item);
    }
}

QString FacesModel::pluginId(int row)
{
    return data(index(row, 0), PluginIdRole).toString();
}

QHash<int, QByteArray> FacesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
 
    roles[ModelDataRole] = "modelData";
    roles[PluginIdRole] = "pluginId";
    return roles;
}

PresetsModel::PresetsModel(QObject *parent)
    : QStandardItemModel(parent)
{}

QString PresetsModel::pluginId(int row) const
{
    return data(index(row, 0), PluginIdRole).toString();
}

QHash<int, QByteArray> PresetsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    roles[ModelDataRole] = "modelData";
    roles[PluginIdRole] = "pluginId";
    roles[ConfigRole] = "config";
    roles[WritableRole] = "writable";
    return roles;
}

SystemMonitor::SystemMonitor(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args)
{
    setHasConfigurationInterface(true);

    m_facePackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/SensorApplet"), QStringLiteral("org.kde.ksysguard.linechart"));

    //Don't set the preset right now as we can't write on the config here because we don't have a Corona yet
    if (args.count() > 2 && args.mid(3).length() > 0) {
        const QString preset = args.mid(3).first().toString();
        if (preset.length() > 0) {
            m_pendingStartupPreset = preset;
        }
    }
}

SystemMonitor::~SystemMonitor()
= default;

void SystemMonitor::init()
{
    configChanged();

    // NOTE: taking the pluginId this way, we take it from the child applet (cpu monitor, memory, whatever) rather than the parent fallback applet (systemmonitor)
    const QString pluginId = KPluginMetaData(kPackage().path() + QStringLiteral("metadata.desktop")).pluginId();

    //FIXME: better way to get the engine (KF6?)
    KDeclarative::QmlObjectSharedEngine *qmlObject = new  KDeclarative::QmlObjectSharedEngine();
    KConfigGroup cg = config();
    m_sensorFaceController = new SensorFaceController(cg, qmlObject->engine());
    qmlObject->deleteLater();

    if (!m_pendingStartupPreset.isNull()) {
        setCurrentPreset(m_pendingStartupPreset);
    } else {
        //Take it from the config, which is *not* accessible from plasmoid.config as is not in config.xml
        const QString preset = config().readEntry("CurrentPreset", pluginId);
        setCurrentPreset(preset);
        if (preset.isEmpty()) {
            resetToCustomPreset();
            emit currentPresetChanged();
        }
    }
}

SensorFaceController *SystemMonitor::faceController() const
{
    return m_sensorFaceController;
}

void SystemMonitor::configChanged()
{
    setFace(configScheme()->property("chartFace").toString());
}

void SystemMonitor::setFace(const QString &face)
{
    if (face.length() == 0 || face == m_face || face.contains("..")) {
        return;
    }

    // Valid face?
    const QString oldPath = m_facePackage.path();
    m_facePackage.setPath(face);
    if (!m_facePackage.isValid()) {
        m_facePackage.setPath(oldPath);
        return;
    }

    m_face = face;

    delete m_faceMetadata;
    m_faceMetadata = new KDesktopFile(m_facePackage.path() + QStringLiteral("metadata.desktop"));

    const QString xmlPath = m_facePackage.filePath("mainconfigxml");

    if (!xmlPath.isEmpty()) {
        QFile file(xmlPath);
        KConfigGroup cg = config();
        cg = KConfigGroup(&cg, face);
        if (m_faceConfiguration) {
            m_faceConfiguration->deleteLater();
        }
        if (m_faceConfigLoader) {
            m_faceConfigLoader->deleteLater();
        }
        m_faceConfigLoader = new KConfigLoader(cg, &file, this);
        m_faceConfiguration = new KDeclarative::ConfigPropertyMap(m_faceConfigLoader, this);
    }
    
    emit faceChanged();
}

QString SystemMonitor::face() const
{
    return m_face;
}

QString SystemMonitor::faceName() const
{
    if (!m_faceMetadata) {
        return QString();
    }
    return m_faceMetadata->readName();
}

QString SystemMonitor::faceIcon() const
{
    if (!m_faceMetadata) {
        return QString();
    }
    return m_faceMetadata->readIcon();
}

QString SystemMonitor::currentPreset() const
{
    return m_currentPreset;
}

void SystemMonitor::setCurrentPreset(const QString &preset)
{
    if (preset == m_currentPreset) {
        return;
    }

    m_currentPreset = preset;
    config().writeEntry("CurrentPreset", preset);

    if (preset.isEmpty()) {
        resetToCustomPreset();
        emit currentPresetChanged();
        return;
    }

    auto presetPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));

    presetPackage.setPath(preset);

    if (!presetPackage.isValid()) {
        return;
    }

    if (presetPackage.metadata().value(QStringLiteral("X-Plasma-RootPath")) != QStringLiteral("org.kde.plasma.systemmonitor")) {
        return;
    }

    disconnect(configScheme(), &KCoreConfigSkeleton::configChanged, this, &SystemMonitor::resetToCustomPresetUserConfiguring);
    disconnect(m_faceConfigLoader, &KCoreConfigSkeleton::configChanged, this, &SystemMonitor::resetToCustomPresetUserConfiguring);

    KDesktopFile df(presetPackage.path() + QStringLiteral("metadata.desktop"));
    KConfigGroup configGroup(df.group("Config"));

    // Load the title
    KConfigSkeletonItem *item = configScheme()->findItemByName(QStringLiteral("title"));
    if (item) {
        item->setProperty(df.readName());
        configScheme()->save();
        //why read? read will update KConfigSkeletonItem::mLoadedValue,
        //allowing a write operation to be performed next time
        configScheme()->read();
    }

    //Remove the "custon" value from presets models
    if (m_availablePresetsModel &&
        m_availablePresetsModel->data(m_availablePresetsModel->index(0, 0), PresetsModel::PluginIdRole).toString().isEmpty()) {
        m_availablePresetsModel->removeRow(0);
    }

    // Load the global config keys
    for (const QString &key : configGroup.keyList()) {
        KConfigSkeletonItem *item = configScheme()->findItemByName(key);

        if (item) {
            if (item->property().type() == QVariant::StringList) {
                // Special case: sensor ids or textOnlySensorIds can have wildchars that need to be expanded
                if (key == "sensorIds" || key == "textOnlySensorIds") {
                    const QStringList partialEntries = configGroup.readEntry(key, QStringList());
                    QStringList sensors;

                    for (const QString &id : partialEntries) {
                        KSysGuard::SensorQuery query{id};
                        query.execute();
                        query.waitForFinished();

                        sensors.append(query.sensorIds());
                    }
                    item->setProperty(QVariant::fromValue(sensors));
                } else {
                    item->setProperty(QVariant::fromValue(configGroup.readEntry(key, QStringList())));
                }
            } else {
                const QString &value = configGroup.readEntry(key);
                if (key == QStringLiteral("chartFace") && value.length() > 0) {
                    setFace(value);
                }
                item->setProperty(value);
            }
            configScheme()->save();
            //why read? read will update KConfigSkeletonItem::mLoadedValue,
            //allowing a write operation to be performed next time
            configScheme()->read();
        }
    }

    if (m_faceConfigLoader) {
        configGroup = KConfigGroup(df.group("FaceConfig"));
        for (const QString &key : configGroup.keyList()) {
            KConfigSkeletonItem *item = m_faceConfigLoader->findItemByName(key);
            if (item) {
                if (item->property().type() == QVariant::StringList) {
                    item->setProperty(configGroup.readEntry(key, QStringList()));
                } else {
                    item->setProperty(configGroup.readEntry(key));
                }
                m_faceConfigLoader->save();
                m_faceConfigLoader->read();
            }
        }
    }

    emit currentPresetChanged();

    connect(configScheme(), &KCoreConfigSkeleton::configChanged, this, &SystemMonitor::resetToCustomPresetUserConfiguring);
    connect(m_faceConfigLoader, &KCoreConfigSkeleton::configChanged, this, &SystemMonitor::resetToCustomPresetUserConfiguring);
}

void SystemMonitor::resetToCustomPresetUserConfiguring()
{
    // automatically switch to "custom" preset only when the user changes settings from the dialog, *not* nettings changed programmatically by the plasmoid itself
    if (isUserConfiguring()) {
        resetToCustomPreset();
    }
}

void SystemMonitor::resetToCustomPreset()
{
    if (m_availablePresetsModel &&
        !m_availablePresetsModel->data(m_availablePresetsModel->index(0, 0), PresetsModel::PluginIdRole).toString().isEmpty()) {
        QStandardItem *item = new QStandardItem(i18n("Custom"));
            item->setData(i18n("Custom"), PresetsModel::ModelDataRole);
        m_availablePresetsModel->insertRow(0, item);
    }

    setCurrentPreset(QString());
}

QString SystemMonitor::compactRepresentationPath() const
{
    return m_facePackage.filePath("CompactRepresentation");
}

QString SystemMonitor::fullRepresentationPath() const
{
    return m_facePackage.filePath("FullRepresentation");
}

QString SystemMonitor::configPath() const
{
    return m_facePackage.filePath("ConfigUI");
}

SensorFace *SystemMonitor::sensorFullRepresentation()
{
    return m_sensorFaceController->fullRepresentation();
}

bool SystemMonitor::supportsSensorsColors() const
{
    if (!m_faceMetadata) {
        return false;
    }

    KConfigGroup cg(m_faceMetadata, QStringLiteral("Config"));
    return cg.readEntry("SupportsSensorsColors", false);
}

bool SystemMonitor::supportsTotalSensor() const
{
    if (!m_faceMetadata) {
        return false;
    }

    KConfigGroup cg(m_faceMetadata, QStringLiteral("Config"));
    return cg.readEntry("SupportsTotalSensor", false);
}

bool SystemMonitor::supportsTextOnlySensors() const
{
    if (!m_faceMetadata) {
        return false;
    }

    KConfigGroup cg(m_faceMetadata, QStringLiteral("Config"));
    return cg.readEntry("SupportsTextOnlySensors", false);
}

QAbstractItemModel *SystemMonitor::availableFacesModel()
{
    if (m_availableFacesModel) {
        return m_availableFacesModel;
    }

    m_availableFacesModel = new FacesModel(this);
    m_availableFacesModel->load();
    return m_availableFacesModel;
}

QAbstractItemModel *SystemMonitor::availablePresetsModel()
{
    if (m_availablePresetsModel) {
        return m_availablePresetsModel;
    }

    m_availablePresetsModel = new PresetsModel(this);

    // TODO move that into a PresetsModel::load()
    reloadAvailablePresetsModel();

    if (m_currentPreset.isEmpty()) {
        resetToCustomPreset();
    }

    return m_availablePresetsModel;
}

void SystemMonitor::reloadAvailablePresetsModel()
{
    if (!m_availablePresetsModel) {
        availablePresetsModel();
        return;
    }

    m_availablePresetsModel->clear();
    QList<KPluginMetaData> plugins = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/Applet"), QString(), [](const KPluginMetaData &plugin) {
        return plugin.value(QStringLiteral("X-Plasma-RootPath")) == QStringLiteral("org.kde.plasma.systemmonitor");
    });

    QSet<QString> usedNames;

    // We iterate backwards because packages under ~/.local are listed first, while we want them last
    auto it = plugins.rbegin();
    for (; it != plugins.rend(); ++it) {
        const auto &plugin = *it;
        KPackage::Package p = KPackage::PackageLoader::self()->loadPackage("Plasma/Applet", plugin.pluginId());
        KDesktopFile df(p.path() + QStringLiteral("metadata.desktop"));

        QString baseName = df.readName();
        QString name = baseName;
        int id = 0;

        while (usedNames.contains(name)) {
            name = baseName % " (" % QString::number(++id) % ")";
        }
        usedNames << name;

        QStandardItem *item = new QStandardItem(baseName);

        // TODO config
        QVariantMap config;

        KConfigGroup configGroup(df.group("Config"));

        const QStringList keys = configGroup.keyList();
        for (const QString &key : keys) {
            // all strings for now, type conversion happens in QML side when we have the config property map
            config.insert(key, configGroup.readEntry(key));
        }

        item->setData(name, PresetsModel::ModelDataRole);
        item->setData(plugin.pluginId(), PresetsModel::PluginIdRole);
        item->setData(config, PresetsModel::ConfigRole);

        item->setData(QFileInfo(p.path() + QStringLiteral("metadata.desktop")).isWritable(), PresetsModel::WritableRole);

        m_availablePresetsModel->appendRow(item);
    }
}

QObject *SystemMonitor::faceConfiguration() const
{
    return m_sensorFaceController->faceConfiguration();
}

void SystemMonitor::createNewPreset(const QString &pluginName, const QString &comment, const QString &author, const QString &email, const QString &license, const QString &website)
{
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return;
    }

    KConfig c(dir.path() % QLatin1Literal("/metadata.desktop"));

    KConfigGroup cg(&c, "Desktop Entry");
    cg.writeEntry("Name", configScheme()->property("title"));
    cg.writeEntry("Comment", comment);
    cg.writeEntry("Icon", "ksysguardd");
    cg.writeEntry("X-Plasma-API", "declarativeappletscript");
    cg.writeEntry("X-Plasma-MainScript", "ui/main.qml");
    cg.writeEntry("X-Plasma-Provides", "org.kde.plasma.systemmonitor");
    cg.writeEntry("X-Plasma-RootPath", "org.kde.plasma.systemmonitor");
    cg.writeEntry("X-KDE-PluginInfo-Name", pluginName);
    cg.writeEntry("X-KDE-ServiceTypes", "Plasma/Applet");
    cg.writeEntry("X-KDE-PluginInfo-Author", author);
    cg.writeEntry("X-KDE-PluginInfo-Email", email);
    cg.writeEntry("X-KDE-PluginInfo-Website", website);
    cg.writeEntry("X-KDE-PluginInfo-Category", "System Information");
    cg.writeEntry("X-KDE-PluginInfo-License", license);
    cg.writeEntry("X-KDE-PluginInfo-EnabledByDefault", "true");
    cg.writeEntry("X-KDE-PluginInfo-Version", "0.1");
    cg.sync();

    cg = KConfigGroup(&c, "Config");
    for (KConfigSkeletonItem *item : configScheme()->items()) {
        cg.writeEntry(item->key(), item->property());
    }
    cg.sync();

    cg = KConfigGroup(&c, "FaceConfig");
    for (KConfigSkeletonItem *item : m_faceConfigLoader->items()) {
        cg.writeEntry(item->key(), item->property());
    }
    cg.sync();

    auto presetPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));

    auto *job = presetPackage.install(dir.path());

    connect(job, &KJob::finished, this, [this, pluginName] () {
        reloadAvailablePresetsModel();
        setCurrentPreset(pluginName);
    });
}

void SystemMonitor::savePreset()
{
    QString pluginName = QStringLiteral("org.kde.plasma.systemmonitor.preset");
    int suffix = 0;

    auto presetPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));

    do {
        presetPackage.setPath(QString());
        presetPackage.setPath(pluginName + QString::number(++suffix));
    } while (!presetPackage.filePath("metadata").isEmpty());

    pluginName += QString::number(suffix);

    createNewPreset(pluginName, QString(), QString(), QString(), QStringLiteral("LGPL 2.1+"), QString());
}

void SystemMonitor::getNewPresets(QQuickItem *ctx)
{
    return getNewStuff(ctx, QStringLiteral("systemmonitor-presets.knsrc"), i18n("Download New Presets"));
}

void SystemMonitor::uninstallPreset(const QString &pluginId)
{
    auto presetPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"), pluginId);

    if (presetPackage.metadata().value("X-Plasma-RootPath") != QStringLiteral("org.kde.plasma.systemmonitor")) {
        return;
    }

    QDir root(presetPackage.path());
    root.cdUp();
    auto *job = presetPackage.uninstall(pluginId, root.path());

    connect(job, &KJob::finished, this, [this] () {
        reloadAvailablePresetsModel();
    });
}

void SystemMonitor::getNewFaces(QQuickItem *ctx)
{
    getNewStuff(ctx, QStringLiteral("systemmonitor-faces.knsrc"), i18n("Download New Display Styles"));
}

void SystemMonitor::getNewStuff(QQuickItem *ctx, const QString &knsrc, const QString &title)
{
    if (m_newStuffDialog) {
        return;
    }

    m_newStuffDialog = new KNS3::DownloadDialog(knsrc);
    m_newStuffDialog->setWindowTitle(title);
    m_newStuffDialog->setWindowModality(Qt::WindowModal);
    m_newStuffDialog->winId(); // so it creates the windowHandle();
    m_newStuffDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(m_newStuffDialog.data(), &KNS3::DownloadDialog::accepted, this, [this] {
        if (m_availableFacesModel) {
            m_availableFacesModel->load();
        }
        reloadAvailablePresetsModel();
    });

    if (ctx && ctx->window()) {
        m_newStuffDialog->windowHandle()->setTransientParent(ctx->window());
    }

    m_newStuffDialog->show();
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemmonitor, SystemMonitor, "metadata.json")

#include "systemmonitor.moc"
