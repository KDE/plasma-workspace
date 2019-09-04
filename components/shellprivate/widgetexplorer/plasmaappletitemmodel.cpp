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

#include <QStandardPaths>
#include <QFileInfo>
#include <QMimeData>

#include <klocalizedstring.h>
#include <kservicetypetrader.h>
#include <ksycoca.h>
#include <kconfig.h>
#include "config-workspace.h"
#include <KPluginTrader>
#include <KPackage/PackageLoader>
#include <KDeclarative/KDeclarative>

PlasmaAppletItem::PlasmaAppletItem(const KPluginInfo& info):
      AbstractItem(),
      m_info(info),
      m_runningCount(0),
      m_local(false)
{
    const QString api(m_info.property(QStringLiteral("X-Plasma-API")).toString());
    if (!api.isEmpty()) {
        const QString _f = PLASMA_RELATIVE_DATA_INSTALL_DIR "/plasmoids/" + info.pluginName() + '/';
        QFileInfo dir(QStandardPaths::locate(QStandardPaths::QStandardPaths::GenericDataLocation,
                                                  _f,
                                                  QStandardPaths::LocateDirectory));
        m_local = dir.exists() && dir.isWritable();
    }

    //attrs.insert("recommended", flags & Recommended ? true : false);
    setText(m_info.name() + " - "+ m_info.category().toLower());

    if (QIcon::hasThemeIcon(info.pluginName())) {
        setIcon(QIcon::fromTheme(info.pluginName()));
    } else if (!m_info.icon().isEmpty()) {
        setIcon(QIcon::fromTheme(info.icon()));
    } else {
        setIcon(QIcon::fromTheme(QStringLiteral("application-x-plasma")));
    }

    //set plugininfo parts as roles in the model, only way qml can understand it
    setData(info.name(), PlasmaAppletItemModel::NameRole);
    setData(info.pluginName(), PlasmaAppletItemModel::PluginNameRole);
    setData(info.comment(), PlasmaAppletItemModel::DescriptionRole);
    setData(info.category().toLower(), PlasmaAppletItemModel::CategoryRole);
    setData(info.license(), PlasmaAppletItemModel::LicenseRole);
    setData(info.website(), PlasmaAppletItemModel::WebsiteRole);
    setData(info.version(), PlasmaAppletItemModel::VersionRole);
    setData(info.author(), PlasmaAppletItemModel::AuthorRole);
    setData(info.email(), PlasmaAppletItemModel::EmailRole);
    setData(0, PlasmaAppletItemModel::RunningRole);
    setData(m_local, PlasmaAppletItemModel::LocalRole);
}

QString PlasmaAppletItem::pluginName() const
{
    return m_info.pluginName();
}

QString PlasmaAppletItem::name() const
{
    return m_info.name();
}

QString PlasmaAppletItem::description() const
{
    return m_info.comment();
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
    return m_info.author();
}

QString PlasmaAppletItem::email() const
{
    return m_info.email();
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
    if (m_info.service()) {
        const QStringList keywords = m_info.property(QStringLiteral("Keywords")).toStringList();
        foreach (const QString &keyword, keywords) {
            if (keyword.startsWith(pattern, Qt::CaseInsensitive)) {
                return true;
            }
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
        //null = not yet done, empty = tried and failed
        if (m_screenshot.isNull()) {
            KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));
            pkg.setDefaultPackageRoot(QStringLiteral("plasma/plasmoids"));
            pkg.setPath(m_info.pluginName());
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
        //null = not yet done, empty = tried and failed
        if (m_icon.isNull()) {
            KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Applet"));
            pkg.setDefaultPackageRoot(QStringLiteral("plasma/plasmoids"));
            pkg.setPath(m_info.pluginName());
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

//PlasmaAppletItemModel

PlasmaAppletItemModel::PlasmaAppletItemModel(QObject * parent)
    : QStandardItemModel(parent),
      m_startupCompleted(false)
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
    //qDebug() << "populating model, our application is" << m_application;

    //qDebug() << "number of applets is"
    //         <<  Plasma::Applet::listAppletInfo(QString(), m_application).count();

    QString constraint;
    bool first = true;
    foreach (const QString prov, m_provides) {
        if (!first) {
            constraint += QLatin1String(" or ");
        }

        first = false;
        constraint += "'" + prov + "' in [X-Plasma-Provides]";
    }

    KPluginInfo::List list = KPluginInfo::fromMetaData(KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma/Applet"), QStringLiteral("plasma/plasmoids")).toVector());

    KPluginTrader::applyConstraints(list, constraint);

    for (auto info : list) {
        //qDebug() << info.pluginName() << "NoDisplay" << info.property("NoDisplay").toBool();
        if (!info.isValid() || info.property(QStringLiteral("NoDisplay")).toBool() || info.category() == QLatin1String("Containments")) {
            // we don't want to show the hidden category
            continue;
        }

        bool inFormFactor = true;

        foreach (const QString &formFactor, KDeclarative::KDeclarative::runtimePlatform()) {
            if (!info.formFactors().isEmpty() &&
                !info.formFactors().contains(formFactor)) {
                inFormFactor = false;
            }
        }
        if (!inFormFactor) {
            continue;
        }

        //qDebug() << info.pluginName() << " is the name of the plugin at" << info.entryPath();
        //qDebug() << info.name() << info.property("X-Plasma-Thumbnail");

        appendRow(new PlasmaAppletItem(info));
    }

    emit modelPopulated();
}

void PlasmaAppletItemModel::setRunningApplets(const QHash<QString, int> &apps)
{
    //foreach item, find that string and set the count
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
    for (int r=0; r<rowCount(); ++r) {
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
    //qDebug() << "GETTING MIME DATA\n";
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
    foreach (const QModelIndex &index, indexes) {
        if (index.row() == lastRow) {
            continue;
        }

        lastRow = index.row();
        PlasmaAppletItem *selectedItem = (PlasmaAppletItem *) itemFromIndex(index);
        appletNames += '\n' + selectedItem->pluginName().toUtf8();
        //qDebug() << selectedItem->pluginName() << index.column() << index.row();
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

