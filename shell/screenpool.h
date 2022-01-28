/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>

#include <KConfigGroup>
#include <KSharedConfig>

class QScreen;
class PrimaryOutputWatcher;

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

    QScreen *screenForId(int id) const;
    QScreen *screenForConnector(const QString &connector);

    int firstAvailableId() const;

    // all ids that are known, included screens not enabled at the moment
    QList<int> knownIds() const;

Q_SIGNALS:
    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);

private:
    void save();
    bool isOutputRedundant(QScreen *screen) const;
    void reconsiderOutputs();
    bool noRealOutputsConnected() const;
    bool isOutputFake(QScreen *screen) const;

    void handleScreenAdded(QScreen *screen);
    void handleScreenRemoved(QScreen *screen);
    void primaryOutputNameChanged(const QString &oldOutputName, const QString &newOutputName);

    KConfigGroup m_configGroup;
    QString m_primaryConnector;
    // order is important
    QMap<int, QString> m_connectorForId;
    QHash<QString, int> m_idForConnector;

    QSet<QScreen *> m_redundantOutputs;

    QTimer m_reconsiderOutputsTimer;
    QTimer m_configSaveTimer;
    PrimaryOutputWatcher *const m_primaryWatcher;
};
