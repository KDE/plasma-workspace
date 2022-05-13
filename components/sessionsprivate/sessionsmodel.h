/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QAbstractListModel>

#include <kdisplaymanager.h>

#include <functional>

class OrgFreedesktopScreenSaverInterface;
namespace org
{
namespace freedesktop
{
using ScreenSaver = ::OrgFreedesktopScreenSaverInterface;
}
}

struct SessionEntry {
    QString realName;
    QString icon;
    QString name;
    QString displayNumber;
    QString session;
    int vtNumber;
    bool isTty;
};

class KDisplayManager;

// This model should be compatible with SDDM::SessionModel
class SessionsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool canSwitchUser READ canSwitchUser CONSTANT)
    Q_PROPERTY(bool canStartNewSession READ canStartNewSession CONSTANT)
    Q_PROPERTY(bool shouldLock READ shouldLock NOTIFY shouldLockChanged)
    Q_PROPERTY(bool showNewSessionEntry MEMBER m_showNewSessionEntry WRITE setShowNewSessionEntry NOTIFY showNewSessionEntryChanged)
    Q_PROPERTY(bool includeUnusedSessions READ includeUnusedSessions WRITE setIncludeUnusedSessions NOTIFY includeUnusedSessionsChanged)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit SessionsModel(QObject *parent = nullptr);
    ~SessionsModel() override = default;

    enum UserRoles {
        NameRole = Qt::UserRole + 1,
        RealNameRole,
        IconRole, // path to a file
        IconNameRole, // name of an icon
        DisplayNumberRole,
        VtNumberRole,
        SessionRole,
        IsTtyRole,
    };
    Q_ENUM(UserRoles)

    bool canSwitchUser() const;
    bool canStartNewSession() const;
    bool shouldLock() const;
    bool includeUnusedSessions() const;

    void setShowNewSessionEntry(bool showNewSessionEntry);
    void setIncludeUnusedSessions(bool includeUnusedSessions);

    Q_INVOKABLE void reload();
    Q_INVOKABLE void switchUser(int vt, bool shouldLock = false);
    Q_INVOKABLE void startNewSession(bool shouldLock = false);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void shouldLockChanged();
    void showNewSessionEntryChanged();
    void countChanged();
    void includeUnusedSessionsChanged();

    void switchedUser(int vt);
    void startedNewSession();
    void aboutToLockScreen();

private:
    void checkScreenLocked(const std::function<void(bool)> &cb);

    KDisplayManager m_displayManager;

    QVector<SessionEntry> m_data;

    bool m_shouldLock = true;

    int m_pendingVt = 0;
    bool m_pendingReserve = false;

    bool m_showNewSessionEntry = false;
    bool m_includeUnusedSessions = true;

    org::freedesktop::ScreenSaver *m_screensaverInterface = nullptr;
};
