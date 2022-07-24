/*
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kcategorizeditemsviewmodels_p.h"
#include <KConfigGroup>
#include <KPluginMetaData>

class PlasmaAppletItemModel;

/**
 * Implementation of the KCategorizedItemsViewModels::AbstractItem
 */
class PlasmaAppletItem : public KCategorizedItemsViewModels::AbstractItem
{
public:
    explicit PlasmaAppletItem(const KPluginMetaData &info);

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
    bool matches(const QString &pattern) const override;
    QStringList keywords() const override;

    // set how many instances of this applet are running
    void setRunning(int count) override;
    bool passesFiltering(const KCategorizedItemsViewModels::Filter &filter) const override;
    QMimeData *mimeData() const;
    QStringList mimeTypes() const;

private:
    KPluginMetaData m_info;
    QString m_screenshot;
    QString m_icon;
    int m_runningCount;
    bool m_local;
};

class PlasmaAppletItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PluginNameRole = Qt::UserRole + 2,
        DescriptionRole = Qt::UserRole + 3,
        CategoryRole = Qt::UserRole + 4,
        LicenseRole = Qt::UserRole + 5,
        WebsiteRole = Qt::UserRole + 6,
        VersionRole = Qt::UserRole + 7,
        AuthorRole = Qt::UserRole + 8,
        EmailRole = Qt::UserRole + 9,
        RunningRole = Qt::UserRole + 10,
        LocalRole = Qt::UserRole + 11,
        ScreenshotRole = Qt::UserRole + 12,
    };

    explicit PlasmaAppletItemModel(QObject *parent = nullptr);

    QStringList mimeTypes() const override;
    QSet<QString> categories() const;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void setApplication(const QString &app);
    void setRunningApplets(const QHash<QString, int> &apps);
    void setRunningApplets(const QString &name, int count);

    QString &Application();

    QStringList provides() const;
    void setProvides(const QStringList &provides);

    QHash<int, QByteArray> roleNames() const override;

    bool startupCompleted() const;
    void setStartupCompleted(bool complete);

Q_SIGNALS:
    void modelPopulated();

private:
    QString m_application;
    QStringList m_provides;
    KConfigGroup m_configGroup;
    bool m_startupCompleted : 1;

private Q_SLOTS:
    void populateModel();
};
