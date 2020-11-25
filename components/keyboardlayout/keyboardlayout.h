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

    Q_PROPERTY(QString layoutDisplayName
               MEMBER mLayoutDisplayName
               NOTIFY layoutDisplayNameChanged)

    Q_PROPERTY(QString layoutLongName
               MEMBER mLayoutLongName
               NOTIFY layoutLongNameChanged)

    Q_PROPERTY(QStringList layouts
               MEMBER mLayouts
               NOTIFY layoutsChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void layoutDisplayNameChanged();
    void layoutLongNameChanged();
    void layoutsChanged();

protected Q_SLOTS:
    void switchToNextLayout();
    void switchToPreviousLayout();

private:
    enum DBusData {LayoutDisplayName, LayoutLongName, Layouts};

    template<class T>
    void requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)());
    template<DBusData>
    inline void requestDBusData();

    QString mLayoutDisplayName;
    QString mLayoutLongName;
    QStringList mLayouts;
    OrgKdeKeyboardLayoutsInterface *mIface;
};

#endif // KEYBOARDLAYOUT_H
