/*
    SPDX-FileCopyrightText: 2021 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickView>

#include <plasma/containment.h>
#include <plasma/plasma.h>

namespace KActivities
{
class Consumer;
class Info;
}

class ShellCorona;
class ShellContainmentModel;

class ScreenPoolModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ScreenPoolModelRoles { ScreenIdRole = Qt::UserRole + 1, ScreenNameRole, ContainmentsRole, PrimaryRole, EnabledRole };

public:
    explicit ScreenPoolModel(ShellCorona *corona, QObject *parent = nullptr);
    ~ScreenPoolModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void load();
    void remove(int screenId);

private:
    ShellCorona *m_corona;
    struct Data {
        int id;
        QString name;
        bool primary;
        bool enabled;
    };
    QTimer *m_reloadTimer = nullptr;
    QVector<Data> m_screens;
    QVector<ShellContainmentModel *> m_containments;
};

class ShellContainmentModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(ScreenPoolModel *screenPoolModel READ screenPoolModel CONSTANT)

public:
    enum ShellContainmentModelRoles {
        ContainmentIdRole = Qt::UserRole + 1,
        NameRole,
        ScreenRole,
        EdgeRole,
        EdgePositionRole,
        PanelCountAtRightRole,
        PanelCountAtTopRole,
        PanelCountAtLeftRole,
        PanelCountAtBottomRole,
        ActivityRole,
        IsActiveRole,
        ImageSourceRole,
        DestroyedRole
    };

public:
    explicit ShellContainmentModel(ShellCorona *corona, int screenId, ScreenPoolModel *parent = nullptr);
    ~ShellContainmentModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    ScreenPoolModel *screenPoolModel() const;

    Q_INVOKABLE void remove(int contId);
    Q_INVOKABLE void moveContainementToScreen(unsigned int contId, int newScreen);

    bool findContainment(unsigned int containmentId) const;

    void loadActivitiesInfos();

public Q_SLOTS:
    void load();

private:
    static QString plasmaLocationToString(const Plasma::Types::Location location);
    static QString containmentTypeToString(const Plasma::Types::ContainmentType containmentType);

    Plasma::Containment *containmentById(unsigned int id);
    QString containmentPreview(Plasma::Containment *containment);

private:
    int m_screenId = -1;
    ShellCorona *m_corona;
    struct Data {
        unsigned int id;
        QString name;
        int screen;
        Plasma::Types::Location edge;
        QString activity;
        bool changed = false;
        bool isActive = true;
        QString image;
        const Plasma::Containment *containment;
    };
    QTimer *m_reloadTimer = nullptr;
    QVector<Data> m_containments;
    ScreenPoolModel *m_screenPoolModel;
    QHash<QString, KActivities::Info *> m_activitiesInfos;
    KActivities::Consumer *m_activityConsumer;
    QHash<int, QHash<Plasma::Types::Location, QList<int>>> m_edgeCount;
};

class ShellContainmentConfig : public QQmlApplicationEngine
{
    Q_OBJECT

public:
    ShellContainmentConfig(ShellCorona *corona, QWindow *parent = nullptr);
    ~ShellContainmentConfig() override;

    void init();

private:
    ShellCorona *m_corona;
    ScreenPoolModel *m_model;
};
