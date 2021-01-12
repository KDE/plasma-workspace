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
class LayoutNames;

class KeyboardLayout : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void layoutChanged(uint index);
    void layoutsChanged(QVector<LayoutNames> layouts);

    void layoutLongNameChanged(QString longName);

protected:
    Q_INVOKABLE void switchToNextLayout();
    Q_INVOKABLE void switchToPreviousLayout();
    Q_INVOKABLE void setLayout(uint index);

    Q_INVOKABLE void requestLayoutLongName();

private:
    enum DBusData {Layout, LayoutLongName, Layouts};

    template<class T>
    void requestDBusData(QDBusPendingReply<T> pendingReply, void (KeyboardLayout::*notify)(T));
    template<DBusData>
    void requestDBusData();

    OrgKdeKeyboardLayoutsInterface *mIface;
};

#endif // KEYBOARDLAYOUT_H
