/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

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

/**
 * ScreenPool acts as a interface on top of QGuiApplication::screens
 * It combines metadata about screen ordering to return an ordered list
 *
 * It also filters out the "fake" screen provided by Qt when no screens are attached.
 */
class ScreenPool : public QObject
{
    Q_OBJECT

public:
    explicit ScreenPool(QObject *parent = nullptr);
    ~ScreenPool() override;

    std::optional<uint> idForName(const QString &connector) const;
    std::optional<uint> idForScreen(const QScreen *screen) const;

    QScreen *screenForId(uint id) const;

    QList<QScreen *> screenOrder() const;
    QScreen *primaryScreen() const;

Q_SIGNALS:
    /**
     * Notify that a screen is about to be removed, this is emitted before changing screenOrder
     */
    void screenRemoved(QScreen *screen); // TODO: necessary?
    void screenOrderChanged(const QList<QScreen *> &screens);

private:
    void handleScreenRemoved(QScreen *screen);
    void handleUpdate();
    QList<QScreen *> m_availableScreens; // Those are all the screen that are available to Corona, ordered by id coming from the protocol
    OutputOrderWatcher *m_outputOrderWatcher;
    friend QDebug operator<<(QDebug d, const ScreenPool *pool);
};

QDebug operator<<(QDebug d, const ScreenPool *pool);
