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

#include <KStandardDirs>
#include <KSycoca>

PlasmaAppletItem::PlasmaAppletItem(PlasmaAppletItemModel *model,
                                   const KPluginInfo& info,
                                   FilterFlags flags)
    : QObject(model),
      m_model(model),
      m_info(info),
      m_runningCount(0),
      m_favorite(flags & Favorite),
      m_local(false)
{
    const QString api(m_info.property("X-Plasma-API").toString());
    if (!api.isEmpty()) {
        QDir dir(KStandardDirs::locateLocal("data", "plasma/plasmoids/" + info.pluginName() + '/', false));
        m_local = dir.exists();
    }

    //attrs.insert("recommended", flags & Recommended ? true : false);
    setText(m_info.name() + " - "+ m_info.category().toLower());

    const QString iconName = m_info.icon().isEmpty() ? "application-x-plasma" : info.icon();
    KIcon icon(iconName);
    setIcon(icon);

    //set plugininfo parts as roles in the model, only way qml can understand it
    setData(info.name(), PlasmaAppletItemModel::NameRole);
    setData(info.pluginName(), PlasmaAppletItemModel::PluginNameRole);
    setData(info.comment(), PlasmaAppletItemModel::DescriptionRole);
    setData(info.category().toLower(), PlasmaAppletItemModel::CategoryRole);
    setData(info.fullLicense().name(KAboutData::FullName), PlasmaAppletItemModel::LicenseRole);
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
        const QStringList keywords = m_info.service()->keywords();
        foreach (const QString &keyword, keywords) {
            if (keyword.startsWith(pattern, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }

    return AbstractItem::matches(pattern);
}

bool PlasmaAppletItem::isFavorite() const
{
    return m_favorite;
}

void PlasmaAppletItem::setFavorite(bool favorite)
{
    if (m_favorite != favorite) {
        m_favorite = favorite;
        m_model->setFavorite(m_info.pluginName(), favorite);
        emitDataChanged();
    }
}

bool PlasmaAppletItem::isLocal() const
{
    return m_local;
}

bool PlasmaAppletItem::passesFiltering(const KCategorizedItemsViewModels::Filter &filter) const
{
    if (filter.first == "running") {
        return running();
    } else if (filter.first == "category") {
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
    types << QLatin1String("text/x-plasmoidservicename");
    return types;
}

PlasmaAppletItemModel* PlasmaAppletItem::appletItemModel()
{
    return m_model;
}

//PlasmaAppletItemModel

PlasmaAppletItemModel::PlasmaAppletItemModel(QObject * parent)
    : QStandardItemModel(parent)
{
    KConfig config("plasmarc");
    m_configGroup = KConfigGroup(&config, "Applet Browser");
    m_favorites = m_configGroup.readEntry("favorites").split(',');
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(populateModel(QStringList)));

    //This is to make QML that is understand it
    QHash<int, QByteArray> newRoleNames = roleNames();
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

    setRoleNames(newRoleNames);

    setSortRole(Qt::DisplayRole);
}

void PlasmaAppletItemModel::populateModel(const QStringList &whatChanged)
{
    if (!whatChanged.isEmpty() && !whatChanged.contains("services")) {
        return;
    }

    clear();
    //kDebug() << "populating model, our application is" << m_application;

    //kDebug() << "number of applets is"
    //         <<  Plasma::Applet::listAppletInfo(QString(), m_application).count();
    foreach (const KPluginInfo &info, Plasma::Applet::listAppletInfo(QString(), m_application)) {
        //kDebug() << info.pluginName() << "NoDisplay" << info.property("NoDisplay").toBool();
        if (info.property("NoDisplay").toBool() || info.category() == i18n("Containments")) {
            // we don't want to show the hidden category
            continue;
        }

        //kDebug() << info.pluginName() << " is the name of the plugin at" << info.entryPath();
        //kDebug() << info.name() << info.property("X-Plasma-Thumbnail");

        PlasmaAppletItem::FilterFlags flags(PlasmaAppletItem::NoFilter);
        if (m_favorites.contains(info.pluginName())) {
            flags |= PlasmaAppletItem::Favorite;
        }

        appendRow(new PlasmaAppletItem(this, info, flags));
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
            const bool running = apps.value(p->pluginName());
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
    types << QLatin1String("text/x-plasmoidservicename");
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
    //kDebug() << "GETTING MIME DATA\n";
    if (indexes.count() <= 0) {
        return 0;
    }

    QStringList types = mimeTypes();

    if (types.isEmpty()) {
        return 0;
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
        //kDebug() << selectedItem->pluginName() << index.column() << index.row();
    }

    data->setData(format, appletNames);
    return data;
}

void PlasmaAppletItemModel::setFavorite(const QString &plugin, bool favorite)
{
    if (favorite) {
        if (!m_favorites.contains(plugin)) {
            m_favorites.append(plugin);
        }
    } else {
        m_favorites.removeAll(plugin);
    }

    m_configGroup.writeEntry("favorites", m_favorites.join(","));
    m_configGroup.sync();
}

void PlasmaAppletItemModel::setApplication(const QString &app)
{
    m_application = app;
    populateModel();
}

QString &PlasmaAppletItemModel::Application()
{
    return m_application;
}

//#include <plasmaappletitemmodel_p.moc>

