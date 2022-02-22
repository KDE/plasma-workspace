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
    void load();
    ~ScreenPool() override;

    QString primaryConnector() const;

    int id(const QString &connector) const;

    QString connector(int id) const;

    // all ids that are known, included screens not enabled at the moment
    QList<int> knownIds() const;

    // QScreen API
    QList<QScreen *> screens() const;
    QScreen *primaryScreen() const;
    QScreen *screenForId(int id) const;
    QScreen *screenForConnector(const QString &connector);

    bool noRealOutputsConnected() const;

Q_SIGNALS:
    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);
    void primaryScreenChanged(QScreen *oldPrimary, QScreen *newPrimary);

private:
    void save();
    void setPrimaryConnector(const QString &primary);
    void insertScreenMapping(int id, const QString &connector);
    int firstAvailableId() const;

    QScreen *outputRedundantTo(QScreen *screen) const;
    void reconsiderOutputs();
    bool isOutputFake(QScreen *screen) const;

    void insertSortedScreen(QScreen *screen);
    void handleScreenAdded(QScreen *screen);
    void handleScreenRemoved(QScreen *screen);
    void handlePrimaryOutputNameChanged(const QString &oldOutputName, const QString &newOutputName);

    void screenInvariants();

    KConfigGroup m_configGroup;
    QString m_primaryConnector;
    // order is important
    QMap<int, QString> m_connectorForId;
    QHash<QString, int> m_idForConnector;

    // List correspondent to qGuiApp->screens(), but sorted first by size then by Id,
    // determines the screen importance while figuring out the reduntant ones
    QList<QScreen *> m_allSortedScreens;
    // m_availableScreens + m_redundantOutputs + m_fakeOutputs == qGuiApp->screens()
    QList<QScreen *> m_availableScreens; // Those are all the screen that are available to Corona
    QHash<QScreen *, QScreen *> m_redundantScreens;
    QSet<QScreen *> m_fakeScreens;

    QTimer m_reconsiderOutputsTimer;
    QTimer m_configSaveTimer;
    PrimaryOutputWatcher *const m_primaryWatcher;
    friend QDebug operator<<(QDebug d, const ScreenPool *pool);
};

QDebug operator<<(QDebug d, const ScreenPool *pool);
