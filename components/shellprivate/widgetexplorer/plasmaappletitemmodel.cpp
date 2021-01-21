/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "plasmaappletitemmodel_p.h"

#include <QFileInfo>
#include <QMimeData>
#include <QStandardPaths>

#include "config-workspace.h"
#include <KAboutData>
#include <KConfig>
#include <KDeclarative/KDeclarative>
#include <KLocalizedString>
#include <KPackage/PackageLoader>
#include <KSycoca>

PlasmaAppletItem::PlasmaAppletItem(const KPluginMetaData &info)
    : AbstractItem()
    , m_info(info)
    , m_runningCount(0)
    , m_local(false)
{
    const QString api(m_info.value(QStringLiteral("X-Plasma-API")));
    if (!api.isEmpty()) {
        const QString _f = PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/" + info.pluginId() + '/';
        QFileInfo dir(QStandardPaths::locate(QStandardPaths::QStandardPaths::GenericDataLocation, _f, QStandardPaths::LocateDirectory));
        m_local = dir.exists() && dir.isWritable();
    }

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

bool PlasmaAppletItem::matches(const QString &pattern) const
{
    const QString keywordsList = KPluginMetaData::readTranslatedString(m_info.rawData(), QStringLiteral("Keywords"));
    const auto keywords = keywordsList.splitRef(QLatin1Char(';'), Qt::SkipEmptyParts);

    for (const auto &keyword : keywords) {
        if (keyword.startsWith(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return AbstractItem::matches(pattern);
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
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(populateModel(QStringList)));

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
    return newRoleNames;
}

void PlasmaAppletItemModel::populateModel(const QStringList &whatChanged)
{
    if (!whatChanged.isEmpty() && !whatChanged.contains(QLatin1String("services"))) {
        return;
    }

    clear();

    auto filter = [this](const KPluginMetaData &plugin) -> bool {
        const QStringList provides = KPluginMetaData::readStringList(plugin.rawData(), QStringLiteral("X-Plasma-Provides"));

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

        bool inFormFactor = true;

        static const auto formFactors = KDeclarative::KDeclarative::runtimePlatform();
        for (const QString &formFactor : formFactors) {
            if (!plugin.formFactors().isEmpty() && !plugin.formFactors().contains(formFactor)) {
                inFormFactor = false;
            }
        }

        if (!inFormFactor) {
            return false;
        }

        return true;
    };

    const QList<KPluginMetaData> packages =
        KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/Applet"), QStringLiteral("plasma/plasmoids"), filter);

    for (const KPluginMetaData &plugin : packages) {
        appendRow(new PlasmaAppletItem(plugin));
    }

    emit modelPopulated();
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

QSet<QString> PlasmaAppletItemModel::categories() const
{
    QSet<QString> cats;
    for (int r = 0; r < rowCount(); ++r) {
        QStandardItem *i = item(r);
        PlasmaAppletItem *p = dynamic_cast<PlasmaAppletItem *>(i);
        if (p) {
            cats.insert(p->category().toLower());
        }
    }

    return cats;
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

//#include <plasmaappletitemmodel_p.moc>
