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
class OutputOrderWatcher;

class ScreenPool : public QObject
{
    Q_OBJECT

public:
    explicit ScreenPool(QObject *parent = nullptr);
    ~ScreenPool() override;

    int idForName(const QString &connector) const;
    int idForScreen(QScreen *screen) const;

    QScreen *screenForId(int id) const;

    QList<QScreen *> screenOrder() const;
    QScreen *primaryScreen() const;
    bool noRealOutputsConnected() const;

Q_SIGNALS:
    void screenRemoved(QScreen *screen); // TODO: necessary?
    void screenOrderChanged(const QList<QScreen *> &screens);

private:
    void insertScreenMapping(int id, const QString &connector);
    int firstAvailableId() const;

    QScreen *outputRedundantTo(QScreen *screen) const;
    void reconsiderOutputs();
    void reconsiderOutputOrder();
    bool isOutputFake(QScreen *screen) const;

    void insertSortedScreen(QScreen *screen);
    void handleScreenAdded(QScreen *screen);
    void handleScreenRemoved(QScreen *screen);
    void handleOutputOrderChanged(const QStringList &newOrder);
    void handleScreenGeometryChanged(QScreen *screen);

    void screenInvariants();


    // List correspondent to qGuiApp->screens(), but sorted first by size then by Id,
    // determines the screen importance while figuring out the reduntant ones
    QList<QScreen *> m_sizeSortedScreens;
    // This will always be true: m_availableScreens + m_redundantScreens + m_fakeScreens == qGuiApp->screens()
    QList<QScreen *> m_availableScreens; // Those are all the screen that are available to Corona, ordered by id coming from the protocol
    QHash<QScreen *, QScreen *> m_redundantScreens;
    QSet<QScreen *> m_fakeScreens;

    bool m_orderChangedPendingSignal = false;

    OutputOrderWatcher *m_outputOrderWatcher;
    friend QDebug operator<<(QDebug d, const ScreenPool *pool);
};

QDebug operator<<(QDebug d, const ScreenPool *pool);
