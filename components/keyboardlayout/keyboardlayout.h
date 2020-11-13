/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef KEYBOARDLAYOUT_H
#define KEYBOARDLAYOUT_H

#include <QDBusReply>

#include "debug.h"

class OrgKdeKeyboardLayoutsInterface;
class QDBusPendingCallWatcher;

class KeyboardLayout : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentLayout
               MEMBER mCurrentLayout
               WRITE setCurrentLayout
               NOTIFY currentLayoutChanged)

    Q_PROPERTY(QString currentLayoutDisplayName
               MEMBER mCurrentLayoutDisplayName
               NOTIFY currentLayoutDisplayNameChanged)

    Q_PROPERTY(QString currentLayoutShortName
               MEMBER mCurrentLayoutShortName
               NOTIFY currentLayoutShortNameChanged)

    Q_PROPERTY(QStringList layouts
               MEMBER mLayouts
               NOTIFY layoutsChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void currentLayoutChanged();
    void currentLayoutDisplayNameChanged();
    void currentLayoutShortNameChanged();
    void layoutsChanged();

private:
    void setCurrentLayout(const QString &layout);

    enum DBusData {CurrentLayout, CurrentLayoutDisplayName, CurrentLayoutShortName, Layouts};

    template<class T>
    void requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)());
    template<DBusData>
    inline void requestDBusData();

    void onCurrentLayoutChanged(const QString &newLayout);
    void onLayoutListChanged();

    QStringList mLayouts;
    QString mCurrentLayout;
    QString mCurrentLayoutDisplayName;
    QString mCurrentLayoutShortName;
    OrgKdeKeyboardLayoutsInterface *mIface;
};

template<class T>
void KeyboardLayout::requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)())
{
    const QDBusPendingCallWatcher * const watcher = new QDBusPendingCallWatcher(pendingReply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this, &out, notify](QDBusPendingCallWatcher *watcher)
        {
            QDBusPendingReply<T> reply = *watcher;
            if (reply.isError()) {
                qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
            } else {
                out = reply.value();
                emit (this->*notify)();
            }
            watcher->deleteLater();
        }
    );
}

#endif // KEYBOARDLAYOUT_H
