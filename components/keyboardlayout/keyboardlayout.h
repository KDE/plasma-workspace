/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusReply>

#include "debug.h"

class OrgKdeKeyboardLayoutsInterface;
class LayoutNames;

class KeyboardLayout : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint layout MEMBER mLayout WRITE setLayout NOTIFY layoutChanged)

    Q_PROPERTY(const QList<LayoutNames> &layoutsList READ getLayoutsList NOTIFY layoutsListChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void layoutChanged();
    void layoutsListChanged();

protected:
    Q_INVOKABLE void switchToNextLayout();
    Q_INVOKABLE void switchToPreviousLayout();

private Q_SLOTS:
    void initialize();

private:
    void setLayout(uint index);
    const QList<LayoutNames> &getLayoutsList() const
    {
        return mLayoutsList;
    }

    enum DBusData {
        Layout,
        LayoutsList,
    };

    template<class T>
    void requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)());
    template<DBusData>
    void requestDBusData();

    uint mLayout = 0;
    QList<LayoutNames> mLayoutsList;
    OrgKdeKeyboardLayoutsInterface *mIface;
};
