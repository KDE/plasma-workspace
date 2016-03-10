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

#ifndef PLASMA_PLASMAAPPLETITEMMODEL_P_H
#define PLASMA_PLASMAAPPLETITEMMODEL_P_H

#include <kplugininfo.h>
#include <Plasma/Applet>
#include "kcategorizeditemsviewmodels_p.h"

class PlasmaAppletItemModel;

/**
 * Implementation of the KCategorizedItemsViewModels::AbstractItem
 */
class PlasmaAppletItem : public QObject, public KCategorizedItemsViewModels::AbstractItem
{
    Q_OBJECT

public:
    enum FilterFlag {
        NoFilter = 0,
        Favorite = 1
    };

    Q_DECLARE_FLAGS(FilterFlags, FilterFlag)

    PlasmaAppletItem(PlasmaAppletItemModel *model, const KPluginInfo& info, FilterFlags flags = NoFilter);

    QString pluginName() const;
    QString name() const override;
    QString category() const;
    QString description() const override;
    QString license() const;
    QString website() const;
    QString version() const;
    QString author() const;
    QString email() const;
    QVariant data(int role = Qt::UserRole + 1) const override;

    int running() const override;
    bool isLocal() const;
    bool isFavorite() const override;
    void setFavorite(bool favorite) override;
    PlasmaAppletItemModel* appletItemModel();
    bool matches(const QString &pattern) const override;

    //set how many instances of this applet are running
    void setRunning(int count) override;
    bool passesFiltering(const KCategorizedItemsViewModels::Filter & filter) const override;
    QMimeData *mimeData() const;
    QStringList mimeTypes() const;

private:
    PlasmaAppletItemModel * m_model;
    KPluginInfo m_info;
    QString m_screenshot;
    QString m_icon;
    int m_runningCount;
    bool m_favorite;
    bool m_local;
};

class PlasmaAppletItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole+1,
        PluginNameRole = Qt::UserRole+2,
        DescriptionRole = Qt::UserRole+3,
        CategoryRole = Qt::UserRole+4,
        LicenseRole = Qt::UserRole+5,
        WebsiteRole = Qt::UserRole+6,
        VersionRole = Qt::UserRole+7,
        AuthorRole = Qt::UserRole+8,
        EmailRole = Qt::UserRole+9,
        RunningRole = Qt::UserRole+10,
        LocalRole = Qt::UserRole+11,
        ScreenshotRole = Qt::UserRole+12
    };

    explicit PlasmaAppletItemModel(QObject * parent = 0);

    QStringList mimeTypes() const override;
    QSet<QString> categories() const;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void setFavorite(const QString &plugin, bool favorite);
    void setApplication(const QString &app);
    void setRunningApplets(const QHash<QString, int> &apps);
    void setRunningApplets(const QString &name, int count);

    QString &Application();

    QStringList provides() const;
    void setProvides(const QStringList &provides);

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void modelPopulated();

private:
    QString m_application;
    QStringList m_favorites;
    QStringList m_provides;
    KConfigGroup m_configGroup;

private Q_SLOTS:
    void populateModel(const QStringList &whatChanged = QStringList());
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PlasmaAppletItem::FilterFlags)

#endif /*PLASMAAPPLETSMODEL_H_*/
