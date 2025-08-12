/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "klookandfeel.h"
#include "klookandfeelmanager.h"

#include <KNSCore/Entry>
#include <KPackage/Package>
#include <KQuickManagedConfigModule>

#include <QStandardItemModel>

class LookAndFeelData;
class LookAndFeelSettings;

/**
 * The ItemModelRow type provides a convenient way to find the index of an item in a QAbstractItemModel.
 * This can be used to set the currentIndex of a GridView or a ListView, etc.
 */
class ItemModelRow : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int index READ index NOTIFY indexChanged)

public:
    explicit ItemModelRow(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    QString role() const;
    void setRole(const QString &role);

    QVariant value() const;
    void setValue(const QVariant &value);

    int index() const;

Q_SIGNALS:
    void modelChanged();
    void roleChanged();
    void valueChanged();
    void indexChanged();

private:
    void update();

    QPointer<QAbstractItemModel> m_model;
    QString m_role;
    QVariant m_value;
    int m_index = -1;
    bool m_complete = false;
};

/**
 * The LookAndFeelInformation type provides a convenient to extract information about a look-and-feel
 * package from a model by the package id.
 */
class LookAndFeelInformation : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString packageId READ packageId WRITE setPackageId NOTIFY packageIdChanged)
    Q_PROPERTY(QStandardItemModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QUrl preview READ preview NOTIFY previewChanged)
    Q_PROPERTY(KLookAndFeel::Variant variant READ variant NOTIFY variantChanged)

public:
    explicit LookAndFeelInformation(QObject *parent = nullptr);

    QString packageId() const;
    void setPackageId(const QString &packageId);

    QStandardItemModel *model() const;
    void setModel(QStandardItemModel *model);

    QString name() const;
    QUrl preview() const;
    KLookAndFeel::Variant variant() const;

Q_SIGNALS:
    void packageIdChanged();
    void modelChanged();
    void nameChanged();
    void previewChanged();
    void variantChanged();

private:
    void refresh();

    void setName(const QString &name);
    void setPreview(const QUrl &preview);
    void setVariant(const KLookAndFeel::Variant &variant);

    QString m_packageId;
    QPointer<QStandardItemModel> m_model;
    QString m_name;
    QUrl m_preview;
    KLookAndFeel::Variant m_variant;
};

class KCMLookandFeel : public KQuickManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(LookAndFeelSettings *settings READ settings CONSTANT)
    Q_PROPERTY(QStandardItemModel *model READ model CONSTANT)

    Q_PROPERTY(KLookAndFeelManager::Contents themeContents READ themeContents NOTIFY themeContentsChanged)
    Q_PROPERTY(KLookAndFeelManager::Contents selectedContents READ selectedContents WRITE setSelectedContents RESET resetSelectedContents NOTIFY
                   selectedContentsChanged)

    Q_PROPERTY(bool plasmaLocked READ isPlasmaLocked NOTIFY plasmaLockedChanged)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole + 1,
        ScreenshotRole,
        FullScreenPreviewRole,
        DescriptionRole,
        ContentsRole,
        PackagePathRole, //< Package root path
        UninstallableRole, //< Package is installed in local directory
        VariantRole,
    };
    Q_ENUM(Roles)

    KCMLookandFeel(QObject *parent, const KPluginMetaData &data);
    ~KCMLookandFeel() override;

    LookAndFeelSettings *settings() const;
    QStandardItemModel *model() const;

    /**
     * Removes the given row from the LookandFeel items.
     * It calls \QStandardItemModel::removeRow and removes local files
     *
     * @param row the given row from LookandFeel items
     * @param removeDependencies whether the dependencies should also be removed
     * @return Returns true if the row is removed; otherwise returns false.
     */
    Q_INVOKABLE bool removeRow(const QString &pluginId, bool removeDependencies = false);

    Q_INVOKABLE void knsEntryChanged(const KNSCore::Entry &entry);
    Q_INVOKABLE void reloadConfig()
    {
        KQuickManagedConfigModule::load();
    }

    KLookAndFeelManager *lookAndFeel() const
    {
        return m_lnf;
    };
    void addKPackageToModel(const KPackage::Package &pkg);

    bool isSaveNeeded() const override;

    KLookAndFeelManager::Contents themeContents() const;

    KLookAndFeelManager::Contents selectedContents() const;
    void setSelectedContents(KLookAndFeelManager::Contents items);
    void resetSelectedContents();

    bool isPlasmaLocked() const;

public Q_SLOTS:
    void defaults() override;
    void apply();

Q_SIGNALS:
    void showConfirmation();
    void themeContentsChanged();
    void selectedContentsChanged();
    void plasmaLockedChanged();

private:
    // List only packages which provide at least one of the components
    QList<KPackage::Package> availablePackages(const QStringList &components);
    void loadModel();
    int pluginIndex(const QString &pluginName) const;

    LookAndFeelData *const m_data;
    KLookAndFeelManager *const m_lnf;

    KLookAndFeelManager::Contents m_themeContents;
    KLookAndFeelManager::Contents m_selectedContents;

    QStandardItemModel *m_model;
};
