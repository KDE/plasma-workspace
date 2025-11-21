/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <qqmlregistration.h>

#include "urlgrabber.h"

class HistoryModel;
class SystemClipboard;

/**
 * Singleton class for executing ClipCommands in QML
 */
class URLGrabberSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(URLGrabber)

    /**
     * Whether the URL grabber is enabled.
     */
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

    /**
     * The timeout in seconds for hiding the popup window.
     */
    Q_PROPERTY(int popupKillTimeout READ popupKillTimeout NOTIFY popupKillTimeoutChanged)

public:
    explicit URLGrabberSingleton(QObject *parent = nullptr);

    bool enabled() const;
    int popupKillTimeout() const;

    Q_INVOKABLE QList<QVariantMap> matchingActions(const QString &text, bool automaticallyInvoked);
    Q_INVOKABLE void execute(const QString &uuid, const QString &text, const ClipCommand &command, const QStringList &actionCapturedTexts);

Q_SIGNALS:
    void requestShowCurrentActionMenu();
    void enabledChanged();
    void popupKillTimeoutChanged();

private:
    void checkNewData(bool isTop);
    ActionList matchMimeActions(const QString &clipData);
    ActionList matchActions(const QString &text, bool automaticallyInvoked);
    void loadConfig();

    std::shared_ptr<SystemClipboard> m_clip;
    std::shared_ptr<HistoryModel> m_historyModel;

    ActionList m_actions;
    QString m_lastURLGrabberTextSelection;
    QString m_lastURLGrabberTextClipboard;
};
