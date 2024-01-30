/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plasmaappletitemmodel_p.h"

#include <QFileInfo>
#include <QMimeData>
#include <QStandardPaths>
#include <QVersionNumber>

#include "config-workspace.h"
#include <KAboutData>
#include <KConfig>
#include <KJsonUtils>
#include <KLocalizedString>
#include <KPackage/PackageLoader>
#include <KRuntimePlatform>

PlasmaAppletItem::PlasmaAppletItem(const KPluginMetaData &info)
    : AbstractItem()
    , m_info(info)
    , m_runningCount(0)
    , m_local(false)
{
    const QString _f = PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/" + info.pluginId() + '/';
    QFileInfo dir(QStandardPaths::locate(QStandardPaths::QStandardPaths::GenericDataLocation, _f, QStandardPaths::LocateDirectory));
    m_local = dir.exists() && dir.isWritable();

    setText(m_info.name() + " - " + m_info.category().toLower());

    if (QIcon::hasThemeIcon(info.pluginId())) {
        setIcon(QIcon::fromTheme(info.pluginId()));
    } else if (!m_info.iconName().isEmpty()) {
        setIcon(QIcon::fromTheme(info.iconName()));
    } else {
        setIcon(QIcon::fromTheme(QStringLiteral("application-x-plasma")));
    }

    // set plugininfo parts as roles in the model, only way qml can understand it
    setData(name(), PlasmaAppletItemModel::NameRole);
    setData(pluginName(), PlasmaAppletItemModel::PluginNameRole);
    setData(description(), PlasmaAppletItemModel::DescriptionRole);
    setData(category().toLower(), PlasmaAppletItemModel::CategoryRole);
    setData(license(), PlasmaAppletItemModel::LicenseRole);
    setData(website(), PlasmaAppletItemModel::WebsiteRole);
    setData(version(), PlasmaAppletItemModel::VersionRole);
    setData(author(), PlasmaAppletItemModel::AuthorRole);
    setData(email(), PlasmaAppletItemModel::EmailRole);
    setData(apiVersion(), PlasmaAppletItemModel::ApiVersionRole);
    setData(isSupported(), PlasmaAppletItemModel::IsSupportedRole);
    setData(unsupportedMessage(), PlasmaAppletItemModel::UnsupportedMessageRole);
    setData(0, PlasmaAppletItemModel::RunningRole);
    setData(m_local, PlasmaAppletItemModel::LocalRole);
}

QString PlasmaAppletItem::pluginName() const
{
    return m_info.pluginId();
}

QString PlasmaAppletItem::name() const
{
    return m_info.name();
}

QString PlasmaAppletItem::description() const
{
    return m_info.description();
}

QString PlasmaAppletItem::license() const
{
    return m_info.license();
}

QString PlasmaAppletItem::category() const
{
    return m_info.category();
}

QString PlasmaAppletItem::website() const
{
    return m_info.website();
}

QString PlasmaAppletItem::version() const
{
    return m_info.version();
}

QString PlasmaAppletItem::author() const
{
    if (m_info.authors().isEmpty()) {
        return QString();
    }

    return m_info.authors().constFirst().name();
}

QString PlasmaAppletItem::email() const
{
    if (m_info.authors().isEmpty()) {
        return QString();
    }

    return m_info.authors().constFirst().emailAddress();
}

int PlasmaAppletItem::running() const
{
    return m_runningCount;
}

void PlasmaAppletItem::setRunning(int count)
{
    m_runningCount = count;
    setData(count, PlasmaAppletItemModel::RunningRole);
    emitDataChanged();
}

QString PlasmaAppletItem::apiVersion() const
{
    return m_info.value(QStringLiteral("X-Plasma-API-Minimum-Version"));
}

bool PlasmaAppletItem::isSupported() const
{
    QVersionNumber version = QVersionNumber::fromString(apiVersion());
    if (version.majorVersion() != 6 /*PROJECT_VERSION_MAJOR*/) {
        return false;
    } else if (version.minorVersion() > 6 /*PROJECT_VERSION_MINOR*/) {
        return false;
    }
    return true;
}

QString PlasmaAppletItem::unsupportedMessage() const
{
    const QString versionString = apiVersion();
    QVersionNumber version = QVersionNumber::fromString(versionString);

    if (version.isNull()) {
        // TODO: We have to hardcode 6 for now as PROJECT_VERSION_MAJOR is still 5, change it back to PROJECT_VERSION_MAJOR with 6.0
        return i18n(
            "This Widget was written for an unknown older version of Plasma and is not compatible with Plasma %1. Please contact the widget's author for an "
            "updated version.",
            6 /*PROJECT_VERSION_MAJOR*/);
    } else if (version.majorVersion() < 6 /*PROJECT_VERSION_MAJOR*/) {
        return i18n("This Widget was written for Plasma %1 and is not compatible with Plasma %2. Please contact the widget's author for an updated version.",
                    version.majorVersion(),
                    6 /*PROJECT_VERSION_MAJOR*/);
    } else if (version.majorVersion() > 6 /*PROJECT_VERSION_MAJOR*/) {
        return i18n("This Widget was written for Plasma %1 and is not compatible with Plasma %2. Please update Plasma in order to use the widget.",
                    version.majorVersion(),
                    6 /*PROJECT_VERSION_MAJOR*/);
    } else if (version.minorVersion() > PROJECT_VERSION_MINOR) {
        return i18n(
            "This Widget was written for Plasma %1 and is not compatible with the latest version of Plasma. Please update Plasma in order to use the widget.",
            versionString);
    }

    return QString();
}

static bool matchesKeywords(QStringView keywords, const QString &pattern)
{
    const auto l = keywords.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const auto &keyword : l) {
        if (keyword.startsWith(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool PlasmaAppletItem::matches(const QString &pattern) const
{
    const QJsonObject rawData = m_info.rawData();
    if (matchesKeywords(KJsonUtils::readTranslatedString(rawData, QStringLiteral("Keywords")), pattern)) {
        return true;
    }

    // Add English name and keywords so users in other languages won't have to switch IME when searching.
    if (!QLocale().name().startsWith(QLatin1String("en_"))) {
        const QString name(rawData[QStringLiteral("KPlugin")][QStringLiteral("Name")].toString());
        const QString keywords(rawData[QStringLiteral("KPlugin")][QStringLiteral("Name")].toString());
        if (name.startsWith(pattern, Qt::CaseInsensitive) || matchesKeywords(keywords, pattern)) {
            return true;
        }
    }

    return AbstractItem::matches(pattern);
}

QStringList PlasmaAppletItem::keywords() const
{
    const static QString keywordsJsonKey = QStringLiteral("X-KDE-Keywords");
    constexpr QLatin1Char separator(',');

    const QJsonObject rawData = m_info.rawData();
    if (rawData.contains(keywordsJsonKey)) {
        QStringList keywords = m_info.value(keywordsJsonKey).split(separator);
        keywords << KJsonUtils::readTranslatedString(rawData, keywordsJsonKey).split(separator);
        keywords.removeDuplicates();
        return keywords;
    }
    return {};
}

bool PlasmaAppletItem::isLocal() const
{
    return m_local;
}

bool PlasmaAppletItem::passesFiltering(const KCategorizedItemsViewModels::Filter &filter) const
{
    if (filter.first == QLatin1String("running")) {
        return running();
    } else if (filter.first == QLatin1String("local")) {
        return isLocal();
    } else if (filter.first == QLatin1String("category")) {
        return m_info.category().toLower() == filter.second;
    } else {
        return false;
    }
}

QMimeData *PlasmaAppletItem::mimeData() const
{
    QMimeData *data = new QMimeData();
    QByteArray appletName;
    appletName += pluginName().toUtf8();
    data->setData(mimeTypes().at(0), appletName);
    return data;
}

QStringList PlasmaAppletItem::mimeTypes() const
{
    QStringList types;
    types << QStringLiteral("text/x-plasmoidservicename");
    return types;
}

QVariant PlasmaAppletItem::data(int role) const
{
    switch (role) {
    case PlasmaAppletItemModel::ScreenshotRole:
        // null = not yet done, empty = tried and failed
        if (m_screenshot.isNull()) {
            KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));
            pkg.setDefaultPackageRoot(QStringLiteral("plasma/plasmoids"));
            pkg.setPath(m_info.pluginId());
            if (pkg.isValid()) {
                const_cast<PlasmaAppletItem *>(this)->m_screenshot = pkg.filePath("screenshot");
            } else {
                const_cast<PlasmaAppletItem *>(this)->m_screenshot = QString();
            }
        } else if (m_screenshot.isEmpty()) {
            return QVariant();
        }
        return m_screenshot;

    case Qt::DecorationRole: {
        // null = not yet done, empty = tried and failed
        if (m_icon.isNull()) {
            KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));
            pkg.setDefaultPackageRoot(QStringLiteral("plasma/plasmoids"));
            pkg.setPath(m_info.pluginId());
            if (pkg.isValid() && pkg.metadata().iconName().startsWith(QLatin1String("/"))) {
                const_cast<PlasmaAppletItem *>(this)->m_icon = pkg.filePath("", pkg.metadata().iconName().toUtf8());
            } else {
                const_cast<PlasmaAppletItem *>(this)->m_icon = QString();
                return AbstractItem::data(role);
            }
        }
        if (m_icon.isEmpty()) {
            return AbstractItem::data(role);
        }
        return QIcon(m_icon);
    }

    default:
        return AbstractItem::data(role);
    }
}

// PlasmaAppletItemModel

PlasmaAppletItemModel::PlasmaAppletItemModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_startupCompleted(false)
{
    setSortRole(Qt::DisplayRole);
}

QHash<int, QByteArray> PlasmaAppletItemModel::roleNames() const
{
    QHash<int, QByteArray> newRoleNames = QAbstractItemModel::roleNames();
    newRoleNames[NameRole] = "name";
    newRoleNames[PluginNameRole] = "pluginName";
    newRoleNames[DescriptionRole] = "description";
    newRoleNames[CategoryRole] = "category";
    newRoleNames[LicenseRole] = "license";
    newRoleNames[WebsiteRole] = "website";
    newRoleNames[VersionRole] = "version";
    newRoleNames[AuthorRole] = "author";
    newRoleNames[EmailRole] = "email";
    newRoleNames[RunningRole] = "running";
    newRoleNames[LocalRole] = "local";
    newRoleNames[ScreenshotRole] = "screenshot";
    newRoleNames[ApiVersionRole] = "apiVersion";
    newRoleNames[IsSupportedRole] = "isSupported";
    newRoleNames[UnsupportedMessageRole] = "unsupportedMessage";
    return newRoleNames;
}

void PlasmaAppletItemModel::populateModel()
{
    clear();

    auto filter = [this](const KPluginMetaData &plugin) -> bool {
        const QStringList provides = plugin.value(QStringLiteral("X-Plasma-Provides"), QStringList());

        if (!m_provides.isEmpty()) {
            const bool providesFulfilled = std::any_of(m_provides.cbegin(), m_provides.cend(), [&provides](const QString &p) {
                return provides.contains(p);
            });

            if (!providesFulfilled) {
                return false;
            }
        }

        if (!plugin.isValid() || plugin.rawData().value(QStringLiteral("NoDisplay")).toBool() || plugin.category() == QLatin1String("Containments")) {
            // we don't want to show the hidden category
            return false;
        }

        static const auto formFactors = KRuntimePlatform::runtimePlatform();
        // If runtimePlatformis not defined, accept everything
        bool inFormFactor = formFactors.isEmpty();

        for (const QString &formFactor : formFactors) {
            if (plugin.formFactors().isEmpty() || plugin.formFactors().contains(formFactor)) {
                inFormFactor = true;
                break;
            }
        }

        if (!inFormFactor) {
            return false;
        }

        return true;
    };

    QList<KPluginMetaData> packages =
        KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/Applet"), QStringLiteral("plasma/plasmoids"), filter);

    // NOTE: Those 2 extra searches are for pure retrocompatibility, to list old plasmoids
    // Just to give the user the possibility to remove them.
    // Eventually after a year or two, this code can be removed to drop this transition support

    // Search all of those that have a desktop file metadata, those are plasma 5 plasmoids
    QList<KPackage::Package> kPackages = KPackage::PackageLoader::self()->listKPackages(QStringLiteral("Plasma/Applet"), QStringLiteral("plasma/plasmoids"));
    for (const KPackage::Package &package : kPackages) {
        KPluginMetaData data = package.metadata();
        if (package.filePath("metadata").endsWith(QStringLiteral("metadata.desktop")) && filter(data)) {
            appendRow(new PlasmaAppletItem(data));
        }
    }
    // Search all packages that have json metadata but not a correct Plasma/Applet and put them at the end: assume they are plasma5 plasmoids
    packages.append(
        KPackage::PackageLoader::self()->findPackages(QString(), QStringLiteral("plasma/plasmoids"), [&filter](const KPluginMetaData &plugin) -> bool {
            return plugin.value(QStringLiteral("KPackageStructure")) != QStringLiteral("Plasma/Applet") && filter(plugin);
        }));

    for (const KPluginMetaData &plugin : packages) {
        appendRow(new PlasmaAppletItem(plugin));
    }

    Q_EMIT modelPopulated();
}

void PlasmaAppletItemModel::setRunningApplets(const QHash<QString, int> &apps)
{
    // for each item, find that string and set the count
    for (int r = 0; r < rowCount(); ++r) {
        QStandardItem *i = item(r);
        PlasmaAppletItem *p = dynamic_cast<PlasmaAppletItem *>(i);

        if (p) {
            const int running = apps.value(p->pluginName());
            p->setRunning(running);
        }
    }
}

void PlasmaAppletItemModel::setRunningApplets(const QString &name, int count)
{
    for (int r = 0; r < rowCount(); ++r) {
        QStandardItem *i = item(r);
        PlasmaAppletItem *p = dynamic_cast<PlasmaAppletItem *>(i);
        if (p && p->pluginName() == name) {
            p->setRunning(count);
        }
    }
}

QStringList PlasmaAppletItemModel::mimeTypes() const
{
    QStringList types;
    types << QStringLiteral("text/x-plasmoidservicename");
    return types;
}

QMimeData *PlasmaAppletItemModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() <= 0) {
        return nullptr;
    }

    QStringList types = mimeTypes();

    if (types.isEmpty()) {
        return nullptr;
    }

    QMimeData *data = new QMimeData();

    QString format = types.at(0);

    QByteArray appletNames;
    int lastRow = -1;
    for (const QModelIndex &index : indexes) {
        if (index.row() == lastRow) {
            continue;
        }

        lastRow = index.row();
        PlasmaAppletItem *selectedItem = (PlasmaAppletItem *)itemFromIndex(index);
        appletNames += '\n' + selectedItem->pluginName().toUtf8();
        // qDebug() << selectedItem->pluginName() << index.column() << index.row();
    }

    data->setData(format, appletNames);
    return data;
}

QStringList PlasmaAppletItemModel::provides() const
{
    return m_provides;
}

void PlasmaAppletItemModel::setProvides(const QStringList &provides)
{
    if (m_provides == provides) {
        return;
    }

    m_provides = provides;
    if (m_startupCompleted) {
        populateModel();
    }
}

void PlasmaAppletItemModel::setApplication(const QString &app)
{
    m_application = app;
    if (m_startupCompleted) {
        populateModel();
    }
}

bool PlasmaAppletItemModel::startupCompleted() const
{
    return m_startupCompleted;
}

void PlasmaAppletItemModel::setStartupCompleted(bool complete)
{
    m_startupCompleted = complete;
}

QString &PlasmaAppletItemModel::Application()
{
    return m_application;
}

// #include <plasmaappletitemmodel_p.moc>
