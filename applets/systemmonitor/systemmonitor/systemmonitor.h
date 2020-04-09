/***************************************************************************
 *   Copyright (C) 2019 Marco Martin <mart@kde.org>                        *
 *
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

#pragma once

#include <QPointer>
#include <QStandardItemModel>
#include <Plasma/Applet>

#include <KDesktopFile>
#include <KPackage/Package>

#include <KNewStuff3/KNS3/DownloadDialog>

class ApplicationListModel;
class QQuickItem;
class SensorFace;
class SensorFaceController;

class KConfigLoader;

namespace KDeclarative {
    class ConfigPropertyMap;
}

class FacesModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum AdditionalRoles {
        ModelDataRole = Qt::UserRole + 1,
        PluginIdRole
    };
    Q_ENUM(AdditionalRoles)

    FacesModel(QObject *parent = nullptr);
    ~FacesModel() = default;

    void load();

    Q_INVOKABLE QString pluginId(int row);

    QHash<int, QByteArray> roleNames() const override;
};

class PresetsModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum AdditionalRoles {
        ModelDataRole = Qt::UserRole + 1,
        PluginIdRole,
        ConfigRole,
        WritableRole
    };
    Q_ENUM(AdditionalRoles)

    PresetsModel(QObject *parent = nullptr);
    ~PresetsModel() = default;

    Q_INVOKABLE QString pluginId(int row) const; // really needed?

    QHash<int, QByteArray> roleNames() const override;
};

class SystemMonitor : public Plasma::Applet
{
    Q_OBJECT
    Q_PROPERTY(QString face READ face NOTIFY faceChanged)
    Q_PROPERTY(QString faceName READ faceName NOTIFY faceChanged)
    Q_PROPERTY(QString currentPreset READ currentPreset WRITE setCurrentPreset NOTIFY currentPresetChanged)
    Q_PROPERTY(QString faceIcon READ faceIcon NOTIFY faceChanged)
    Q_PROPERTY(QString compactRepresentationPath READ compactRepresentationPath NOTIFY faceChanged)
    Q_PROPERTY(QString fullRepresentationPath READ fullRepresentationPath NOTIFY faceChanged)
    Q_PROPERTY(QString configPath READ configPath NOTIFY faceChanged)

    Q_PROPERTY(bool supportsSensorsColors READ supportsSensorsColors NOTIFY faceChanged)
    Q_PROPERTY(bool supportsTotalSensor READ supportsTotalSensor NOTIFY faceChanged)
    Q_PROPERTY(bool supportsTextOnlySensors READ supportsTextOnlySensors NOTIFY faceChanged)

    Q_PROPERTY(QAbstractItemModel *availableFacesModel READ availableFacesModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *availablePresetsModel READ availablePresetsModel CONSTANT)

    Q_PROPERTY(SensorFace *sensorFullRepresentation READ sensorFullRepresentation CONSTANT)
    /**
     * Configuration object: each config key will be a writable property of this object. property bindings work.
     */
    Q_PROPERTY(QObject *faceConfiguration READ faceConfiguration NOTIFY faceChanged)

    Q_PROPERTY(SensorFaceController *faceController READ faceController CONSTANT)

public:
    SystemMonitor( QObject *parent, const QVariantList &args );
    ~SystemMonitor() override;

    void init() override;

    SensorFaceController *faceController() const;

    // Getter, also for QML
    QString face() const;
    // Setter internal use only
    void setFace(const QString &face);

    QString faceName() const;
    QString faceIcon() const;

    QString currentPreset() const;
    void setCurrentPreset(const QString &preset);

    QString compactRepresentationPath() const;
    QString fullRepresentationPath() const;
    QString configPath() const;

    SensorFace *sensorFullRepresentation();

    bool supportsSensorsColors() const;
    bool supportsTotalSensor() const;
    bool supportsTextOnlySensors() const;

    QAbstractItemModel *availableFacesModel();
    QAbstractItemModel *availablePresetsModel();

    QObject *faceConfiguration() const;

    void reloadAvailablePresetsModel();

    // TODO: should there be a dialog that lets the user insert the metadata?
    void createNewPreset(const QString &pluginName, const QString &comment, const QString &author, const QString &email, const QString &license, const QString &website);

    Q_INVOKABLE void savePreset();
    Q_INVOKABLE void getNewPresets(QQuickItem *ctx);
    Q_INVOKABLE void uninstallPreset(const QString &pluginId);

    Q_INVOKABLE void getNewFaces(QQuickItem *ctx);

public Q_SLOTS:
    void configChanged() override;

Q_SIGNALS:
    void faceChanged();
    void currentPresetChanged();

private:
    void resetToCustomPresetUserConfiguring();
    void resetToCustomPreset();

    void getNewStuff(QQuickItem *ctx, const QString &knsrc, const QString &title);

    SensorFaceController *m_sensorFaceController = nullptr;
    QString m_face;
    QString m_currentPreset;
    QString m_pendingStartupPreset;
    KPackage::Package m_facePackage;
    KDesktopFile *m_faceMetadata = nullptr;
    FacesModel *m_availableFacesModel = nullptr;
    PresetsModel *m_availablePresetsModel = nullptr;
    KConfigLoader *m_faceConfigLoader = nullptr;
    KDeclarative::ConfigPropertyMap *m_faceConfiguration = nullptr;

    QPointer<KNS3::DownloadDialog> m_newStuffDialog;
};

