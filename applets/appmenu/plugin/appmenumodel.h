/*
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QAction>
#include <QPointer>
#include <QRect>
#include <QStringList>
#include <qqmlregistration.h>
#include <tasksmodel.h>

#include <Plasma/Containment>

class QMenu;
class QModelIndex;
class QDBusServiceWatcher;
class KDBusMenuImporter;

class AppMenuModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool menuAvailable READ menuAvailable WRITE setMenuAvailable NOTIFY menuAvailableChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)

    Q_PROPERTY(Plasma::Types::ItemStatus containmentStatus MEMBER m_containmentStatus NOTIFY containmentStatusChanged)
    Q_PROPERTY(QRect screenGeometry READ screenGeometry WRITE setScreenGeometry NOTIFY screenGeometryChanged)

public:
    explicit AppMenuModel(QObject *parent = nullptr);
    ~AppMenuModel() override;

    enum AppMenuRole {
        TextRole = Qt::UserRole + 1, // TODO this should be Qt::DisplayRole
        VisibleRole,
        ActionRole,
    };

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath);

    bool menuAvailable() const;
    void setMenuAvailable(bool set);

    bool visible() const;

    QRect screenGeometry() const;
    void setScreenGeometry(QRect geometry);

Q_SIGNALS:
    void requestActivateIndex(int index);
    void bringToFocus(int index);

private Q_SLOTS:
    void onActiveWindowChanged();
    void setVisible(bool visible);
    void update();

Q_SIGNALS:
    void menuAvailableChanged();
    void modelNeedsUpdate();
    void containmentStatusChanged();
    void screenGeometryChanged();
    void visibleChanged();

private:
    bool m_menuAvailable = false;
    bool m_updatePending = false;
    bool m_visible = true;

    Plasma::Types::ItemStatus m_containmentStatus = Plasma::Types::PassiveStatus;
    TaskManager::TasksModel *m_tasksModel;

    QPointer<QMenu> m_menu;
    QDBusServiceWatcher *m_serviceWatcher;
    QString m_serviceName;
    QString m_menuObjectPath;

    QPointer<KDBusMenuImporter> m_importer;
};
