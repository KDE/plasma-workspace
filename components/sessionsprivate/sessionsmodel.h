/*
    Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#ifndef SESSIONSMODEL_H
#define SESSIONSMODEL_H

#include <QAbstractListModel>

#include <kdisplaymanager.h>

#include <functional>

class OrgFreedesktopScreenSaverInterface;
namespace org {
    namespace freedesktop {
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

    enum class Role {
        RealName = Qt::DisplayRole,
        Icon = Qt::DecorationRole, //path to a file
        Name = Qt::UserRole + 1,
        DisplayNumber,
        VtNumber,
        Session,
        IsTty,
        IconName //name of an icon
    };

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

signals:
    void shouldLockChanged();
    void showNewSessionEntryChanged();
    void countChanged();
    void includeUnusedSessionsChanged();

    void switchedUser(int vt);
    void startedNewSession();
    void aboutToLockScreen();

private:
    void checkScreenLocked(const std::function<void (bool)> &cb);

    KDisplayManager m_displayManager;

    QVector<SessionEntry> m_data;

    bool m_shouldLock = true;

    int m_pendingVt = 0;
    bool m_pendingReserve = false;

    bool m_showNewSessionEntry = false;
    bool m_includeUnusedSessions = true;

    org::freedesktop::ScreenSaver *m_screensaverInterface = nullptr;

};

#endif // SESSIONSMODEL_H
