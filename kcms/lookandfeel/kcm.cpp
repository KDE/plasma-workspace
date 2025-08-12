/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>
    SPDX-FileCopyrightText: 2021 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Dominic Hayes <ferenosdev@outlook.com>
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kcm.h"
#include "klookandfeel.h"
#include "lookandfeeldata.h"
#include "lookandfeelsettings.h"

#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KPackage/PackageLoader>
#include <KPluginFactory>
#include <KService>

#include <QCollator>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

K_PLUGIN_FACTORY_WITH_JSON(KCMLookandFeelFactory, "kcm_lookandfeel.json", registerPlugin<KCMLookandFeel>(); registerPlugin<LookAndFeelData>();)

ItemModelRow::ItemModelRow(QObject *parent)
    : QObject(parent)
{
}

void ItemModelRow::classBegin()
{
}

void ItemModelRow::componentComplete()
{
    update();
    m_complete = true;
}

QAbstractItemModel *ItemModelRow::model() const
{
    return m_model;
}

void ItemModelRow::setModel(QAbstractItemModel *model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, &QAbstractItemModel::modelReset, this, &ItemModelRow::update);
        disconnect(m_model, &QAbstractItemModel::rowsInserted, this, &ItemModelRow::update);
        disconnect(m_model, &QAbstractItemModel::rowsMoved, this, &ItemModelRow::update);
        disconnect(m_model, &QAbstractItemModel::rowsRemoved, this, &ItemModelRow::update);
        disconnect(m_model, &QAbstractItemModel::dataChanged, this, &ItemModelRow::update);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &QAbstractItemModel::modelReset, this, &ItemModelRow::update);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &ItemModelRow::update);
        connect(m_model, &QAbstractItemModel::rowsMoved, this, &ItemModelRow::update);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &ItemModelRow::update);
        connect(m_model, &QAbstractItemModel::dataChanged, this, &ItemModelRow::update);
    }

    if (m_complete) {
        update();
    }

    Q_EMIT modelChanged();
}

QString ItemModelRow::role() const
{
    return m_role;
}

void ItemModelRow::setRole(const QString &role)
{
    if (m_role != role) {
        m_role = role;
        if (m_complete) {
            update();
        }
        Q_EMIT roleChanged();
    }
}

QVariant ItemModelRow::value() const
{
    return m_value;
}

void ItemModelRow::setValue(const QVariant &values)
{
    if (m_value != values) {
        m_value = values;
        if (m_complete) {
            update();
        }
        Q_EMIT valueChanged();
    }
}

int ItemModelRow::index() const
{
    return m_index;
}

void ItemModelRow::update()
{
    int newIndex = -1;
    if (m_model) {
        const auto roles = m_model->roleNames();
        const int roleId = roles.key(m_role.toUtf8(), -1);
        if (roleId != -1) {
            const int rowCount = m_model->rowCount();
            for (int row = 0; row < rowCount; ++row) {
                const QModelIndex modelIndex = m_model->index(row, 0);
                if (modelIndex.data(roleId) == m_value) {
                    newIndex = row;
                    break;
                }
            }
        }
    }

    if (m_index != newIndex) {
        m_index = newIndex;
        Q_EMIT indexChanged();
    }
}

LookAndFeelInformation::LookAndFeelInformation(QObject *parent)
    : QObject(parent)
{
}

QString LookAndFeelInformation::packageId() const
{
    return m_packageId;
}

void LookAndFeelInformation::setPackageId(const QString &packageId)
{
    if (m_packageId != packageId) {
        m_packageId = packageId;
        refresh();
        Q_EMIT packageIdChanged();
    }
}

QStandardItemModel *LookAndFeelInformation::model() const
{
    return m_model;
}

void LookAndFeelInformation::setModel(QStandardItemModel *model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, &QAbstractItemModel::rowsInserted, this, &LookAndFeelInformation::refresh);
        disconnect(m_model, &QAbstractItemModel::rowsRemoved, this, &LookAndFeelInformation::refresh);
        disconnect(m_model, &QAbstractItemModel::modelReset, this, &LookAndFeelInformation::refresh);
    }

    m_model = model;
    refresh();

    if (m_model) {
        connect(m_model, &QAbstractItemModel::rowsInserted, this, &LookAndFeelInformation::refresh);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, &LookAndFeelInformation::refresh);
        connect(m_model, &QAbstractItemModel::modelReset, this, &LookAndFeelInformation::refresh);
    }

    Q_EMIT modelChanged();
}

QString LookAndFeelInformation::name() const
{
    return m_name;
}

void LookAndFeelInformation::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}

QUrl LookAndFeelInformation::preview() const
{
    return m_preview;
}

void LookAndFeelInformation::setPreview(const QUrl &url)
{
    if (m_preview != url) {
        m_preview = url;
        Q_EMIT previewChanged();
    }
}

KLookAndFeel::Variant LookAndFeelInformation::variant() const
{
    return m_variant;
}

void LookAndFeelInformation::setVariant(const KLookAndFeel::Variant &variant)
{
    if (m_variant != variant) {
        m_variant = variant;
        Q_EMIT variantChanged();
    }
}

void LookAndFeelInformation::refresh()
{
    if (!m_model) {
        return;
    }

    for (int i = 0; i < m_model->rowCount(); ++i) {
        const QModelIndex index = m_model->index(i, 0);
        if (index.data(KCMLookandFeel::PluginNameRole) == m_packageId) {
            setName(index.data(Qt::DisplayRole).toString());
            setPreview(index.data(KCMLookandFeel::ScreenshotRole).toUrl());
            setVariant(index.data(KCMLookandFeel::VariantRole).value<KLookAndFeel::Variant>());
            return;
        }
    }
}

KCMLookandFeel::KCMLookandFeel(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new LookAndFeelData(this))
    , m_lnf(new LookAndFeelManager(this))
    , m_themeContents(LookAndFeelManager::Empty)
    , m_selectedContents(LookAndFeelManager::AppearanceSettings)
{
    constexpr char uri[] = "org.kde.private.kcms.lookandfeel";
    qmlRegisterAnonymousType<LookAndFeelSettings>("", 1);
    qmlRegisterAnonymousType<QStandardItemModel>("", 1);
    qmlRegisterUncreatableType<KCMLookandFeel>(uri, 1, 0, "KCMLookandFeel", u"Can't create KCMLookandFeel"_s);
    qmlRegisterUncreatableType<LookAndFeelManager>(uri, 1, 0, "LookandFeelManager", u"Can't create LookandFeelManager"_s);
    qmlRegisterType<LookAndFeelInformation>(uri, 1, 0, "LookAndFeelInformation");
    qmlRegisterType<ItemModelRow>(uri, 1, 0, "ItemModelRow");
    qmlRegisterUncreatableMetaObject(KLookAndFeel::staticMetaObject, uri, 1, 0, "LookAndFeel", QStringLiteral("for enums"));

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[DescriptionRole] = "description";
    roles[ScreenshotRole] = "screenshot";
    roles[FullScreenPreviewRole] = "fullScreenPreview";
    roles[ContentsRole] = "contents";
    roles[PackagePathRole] = "packagePath";
    roles[UninstallableRole] = "uninstallable";
    roles[VariantRole] = "variant";

    m_model->setItemRoleNames(roles);
    loadModel();

    auto handleLookAndFeelPackageChanged = [this]() {
        // When the selected LNF package changes, update the available theme contents
        const int index = pluginIndex(settings()->lookAndFeelPackage());
        const LookAndFeelManager::Contents packageContents = m_model->index(index, 0).data(ContentsRole).value<LookAndFeelManager::Contents>();
        if (m_themeContents != packageContents) {
            m_themeContents = packageContents;
            Q_EMIT themeContentsChanged();
        }
        // And also reset the user selection to the new available contents
        resetSelectedContents();
    };

    connect(settings(), &LookAndFeelSettings::lookAndFeelPackageChanged, handleLookAndFeelPackageChanged);
    handleLookAndFeelPackageChanged();

    connect(m_lnf, &LookAndFeelManager::plasmaLockedChanged, this, &KCMLookandFeel::plasmaLockedChanged);
}

KCMLookandFeel::~KCMLookandFeel()
{
}

void KCMLookandFeel::knsEntryChanged(const KNSCore::Entry &entry)
{
    if (!entry.isValid()) {
        return;
    }
    auto removeItemFromModel = [&entry, this]() {
        if (entry.uninstalledFiles().isEmpty()) {
            return;
        }
        const QString guessedPluginId = QFileInfo(entry.uninstalledFiles().constFirst()).fileName();
        const int index = pluginIndex(guessedPluginId);
        if (index != -1) {
            m_model->removeRows(index, 1);
        }
    };
    if (entry.status() == KNSCore::Entry::Deleted) {
        removeItemFromModel();
    } else if (entry.status() == KNSCore::Entry::Installed && !entry.installedFiles().isEmpty()) {
        if (!entry.uninstalledFiles().isEmpty()) {
            removeItemFromModel(); // In case we updated it we don't want to have it in twice
        }
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        pkg.setPath(entry.installedFiles().constFirst());
        addKPackageToModel(pkg);
    }
}

QStandardItemModel *KCMLookandFeel::model() const
{
    return m_model;
}

bool KCMLookandFeel::removeRow(const QString &pluginId, bool removeDependencies)
{
    const int row = pluginIndex(pluginId);
    if (row == -1) {
        return false;
    }

    const QModelIndex index = m_model->index(row, 0);
    if (!m_model->checkIndex(index) || !index.data(UninstallableRole).toBool()) {
        // Invalid request
        return false;
    }

    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    package.setPath(index.data(PackagePathRole).toString());

    if (!package.isValid()) {
        return false;
    }

    const auto contentsToRemove = removeDependencies ? index.data(ContentsRole).value<LookAndFeelManager::Contents>() : LookAndFeelManager::Empty;
    const bool isRemoved = m_lnf->remove(package, contentsToRemove);

    if (isRemoved) {
        // Remove the theme from the item model
        const bool ret = m_model->removeRow(row);
        Q_ASSERT_X(ret, "removeRow", QStringLiteral("Failed to remove item at row %1").arg(row).toLatin1().constData()); // Shouldn't happen
    }

    return isRemoved;
}

int KCMLookandFeel::pluginIndex(const QString &pluginName) const
{
    const auto results = m_model->match(m_model->index(0, 0), PluginNameRole, pluginName, 1, Qt::MatchExactly);
    if (results.count() == 1) {
        return results.first().row();
    }

    return -1;
}

QList<KPackage::Package> KCMLookandFeel::availablePackages(const QStringList &components)
{
    QList<KPackage::Package> packages;

    const QList<KPluginMetaData> packagesMetaData = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/LookAndFeel"));

    for (const KPluginMetaData &metadata : packagesMetaData) {
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), metadata.pluginId());
        if (components.isEmpty()) {
            packages << pkg;
        } else {
            for (const auto &component : components) {
                if (!pkg.filePath(component.toUtf8()).isEmpty()) {
                    packages << pkg;
                    break;
                }
            }
        }
    }

    return packages;
}

LookAndFeelSettings *KCMLookandFeel::settings() const
{
    return m_data->settings();
}

void KCMLookandFeel::loadModel()
{
    m_model->clear();

    QList<KPackage::Package> pkgs = availablePackages({u"defaults"_s, u"layouts"_s});

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(pkgs.begin(), pkgs.end(), [&collator](const KPackage::Package &a, const KPackage::Package &b) {
        return collator.compare(a.metadata().name(), b.metadata().name()) < 0;
    });

    for (const KPackage::Package &pkg : pkgs) {
        addKPackageToModel(pkg);
    }
}

void KCMLookandFeel::addKPackageToModel(const KPackage::Package &pkg)
{
    if (!pkg.metadata().isValid()) {
        return;
    }
    QStandardItem *row = new QStandardItem(pkg.metadata().name());
    row->setData(pkg.metadata().pluginId(), PluginNameRole);
    row->setData(pkg.metadata().description(), DescriptionRole);
    row->setData(QUrl::fromLocalFile(pkg.filePath("preview")), ScreenshotRole);
    row->setData(pkg.filePath("fullscreenpreview"), FullScreenPreviewRole);
    row->setData(QVariant::fromValue(m_lnf->packageContents(pkg)), ContentsRole);

    row->setData(pkg.path(), PackagePathRole);
    const QString writableLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    row->setData(pkg.path().startsWith(writableLocation), UninstallableRole);

    row->setData(QVariant::fromValue(KLookAndFeel::colorSchemeVariant(pkg)), VariantRole);

    m_model->appendRow(row);
}

bool KCMLookandFeel::isSaveNeeded() const
{
    return settings()->isSaveNeeded();
}

void KCMLookandFeel::defaults()
{
    KQuickManagedConfigModule::defaults();
    Q_EMIT showConfirmation();
}

void KCMLookandFeel::apply()
{
    QString newLnfPackage = settings()->lookAndFeelPackage();
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    package.setPath(newLnfPackage);

    if (!package.isValid()) {
        return;
    }

    save();

    if (!settings()->automaticLookAndFeel()) {
        m_lnf->save(package, m_selectedContents);
    }
}

LookAndFeelManager::Contents KCMLookandFeel::themeContents() const
{
    return m_themeContents;
}

LookAndFeelManager::Contents KCMLookandFeel::selectedContents() const
{
    return m_selectedContents;
}

void KCMLookandFeel::setSelectedContents(LookAndFeelManager::Contents items)
{
    if (selectedContents() == items) {
        return;
    }

    m_selectedContents = items;
    Q_EMIT selectedContentsChanged();
}

void KCMLookandFeel::resetSelectedContents()
{
    // Reset the user selection to those contents provided by the theme.
    LookAndFeelManager::Contents resetContents = m_themeContents;
    // But do not select layout contents by default if there appaerance settings
    if (m_themeContents & LookAndFeelManager::AppearanceSettings) {
        resetContents &= ~LookAndFeelManager::LayoutSettings;
    }
    setSelectedContents(resetContents);
}

bool KCMLookandFeel::isPlasmaLocked() const
{
    return m_lnf->isPlasmaLocked();
}

#include "kcm.moc"
#include "moc_kcm.cpp"
