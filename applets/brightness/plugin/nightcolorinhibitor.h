/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

/**
 * The Inhibitor class provides a convenient way to temporarily disable Night Color.
 */
class NightColorInhibitor : public QObject
{
    Q_OBJECT
    /**
     * This property holds a value to indicate the current state of the inhibitor.
     */
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    explicit NightColorInhibitor(QObject *parent = nullptr);
    ~NightColorInhibitor() override;

    /**
     * This enum type is used to specify the state of the inhibitor.
     */
    enum State {
        Inhibiting, ///< Night Color is being inhibited.
        Inhibited, ///< Night Color is inhibited.
        Uninhibiting, ///< Night Color is being uninhibited.
        Uninhibited, ///< Night Color is uninhibited.
    };
    Q_ENUM(State)

    /**
     * Returns the current state of the inhibitor.
     */
    State state() const;

public Q_SLOTS:
    /**
     * Attempts to temporarily disable Night Color.
     *
     * After calling this method, the inhibitor will enter the Inhibiting state.
     * Eventually, the inhibitor will enter the Inhibited state when the inhibition
     * request has been processed successfully by the Night Color manager.
     *
     * This method does nothing if the inhibitor is in the Inhibited state.
     */
    void inhibit();

    /**
     * Attempts to undo the previous call to inhibit() method.
     *
     * After calling this method, the inhibitor will enter the Uninhibiting state.
     * Eventually, the inhibitor will enter the Uninhibited state when the uninhibition
     * request has been processed by the Night Color manager.
     *
     * This method does nothing if the inhibitor is in the Uninhibited state.
     */
    void uninhibit();

Q_SIGNALS:
    /**
     * Emitted whenever the state of the inhibitor has changed.
     */
    void stateChanged();

private:
    class Private;
    QScopedPointer<Private> d;
};
