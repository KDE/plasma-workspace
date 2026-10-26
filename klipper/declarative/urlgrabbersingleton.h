/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <qqmlregistration.h>

#include "urlgrabber.h"

/**
 * Singleton class for executing ClipCommands in QML
 */
class URLGrabberSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(URLGrabber)

    /**
     * The timeout in seconds for hiding the popup window.
     */
    Q_PROPERTY(int popupKillTimeout READ popupKillTimeout NOTIFY popupKillTimeoutChanged)

public:
    explicit URLGrabberSingleton(QObject *parent = nullptr);

    int popupKillTimeout() const;

    Q_INVOKABLE QList<QVariantMap> matchingActions(const QString &text, bool automaticallyInvoked);
    Q_INVOKABLE void execute(const QString &uuid, const QString &text, const ClipCommand &command, const QStringList &actionCapturedTexts);

Q_SIGNALS:
    void popupKillTimeoutChanged();

private:
    void loadConfig();

    ActionList m_actions;
};
