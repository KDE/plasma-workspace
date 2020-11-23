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

    Q_PROPERTY(QString currentLayoutLongName
               MEMBER mCurrentLayoutLongName
               NOTIFY currentLayoutLongNameChanged)

    Q_PROPERTY(QStringList layouts
               MEMBER mLayouts
               NOTIFY layoutsChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void currentLayoutChanged();
    void currentLayoutDisplayNameChanged();
    void currentLayoutLongNameChanged();
    void layoutsChanged();

protected Q_SLOTS:
    void switchToNextLayout();

private:
    void setCurrentLayout(const QString &layout);

    enum DBusData {CurrentLayout, CurrentLayoutDisplayName, CurrentLayoutLongName, Layouts};

    template<class T>
    void requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)());
    template<DBusData>
    inline void requestDBusData();

    QString mCurrentLayout;
    QString mCurrentLayoutDisplayName;
    QString mCurrentLayoutLongName;
    QStringList mLayouts;
    OrgKdeKeyboardLayoutsInterface *mIface;
};

#endif // KEYBOARDLAYOUT_H
