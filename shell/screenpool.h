/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

#include <KConfigGroup>
#include <KSharedConfig>

class QScreen;

class ScreenPool : public QObject
{
    Q_OBJECT

public:
    explicit ScreenPool(const KSharedConfig::Ptr &config, QObject *parent = nullptr);
    void load(QScreen *primary);
    ~ScreenPool() override;

    QString primaryConnector() const;
    void setPrimaryConnector(const QString &primary);

    void insertScreenMapping(int id, const QString &connector);

    int id(const QString &connector) const;

    QString connector(int id) const;

    int firstAvailableId() const;

    // all ids that are known, included screens not enabled at the moment
    QList<int> knownIds() const;

private:
    void save();

    KConfigGroup m_configGroup;
    QString m_primaryConnector;
    // order is important
    QMap<int, QString> m_connectorForId;
    QHash<QString, int> m_idForConnector;

    QTimer m_configSaveTimer;
};
