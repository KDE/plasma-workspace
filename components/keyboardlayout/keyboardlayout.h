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
               WRITE setCurrentLayout)

    Q_PROPERTY(QString currentLayoutDisplayName
               READ currentLayoutDisplayName
               NOTIFY currentLayoutChanged)

    Q_PROPERTY(QString currentLayoutShortName
               READ currentLayoutShortName
               NOTIFY currentLayoutChanged)

    Q_PROPERTY(QStringList layouts
               MEMBER mLayouts
               NOTIFY layoutsChanged)

public:
    explicit KeyboardLayout(QObject *parent = nullptr);
    ~KeyboardLayout() override;

Q_SIGNALS:
    void currentLayoutChanged();
    void layoutsChanged();

private:
    void onCurrentLayoutChanged(const QString &newLayout);
    void onLayoutListChanged();

    QString currentLayoutShortName() const;
    QString currentLayoutDisplayName() const;
    void setCurrentLayout(const QString &layout);

    template<class T>
    static T callDBus(QDBusPendingReply<T> pendingReply);

    QStringList mLayouts;
    QString mCurrentLayout;
    OrgKdeKeyboardLayoutsInterface *mIface;
};

template<class T>
T KeyboardLayout::callDBus(QDBusPendingReply<T> pendingReply)
{
    pendingReply.waitForFinished();
    if (pendingReply.isError()) {
        qCWarning(KEYBOARD_LAYOUT) << pendingReply.error().message();
        return {};
    } else {
        return pendingReply.value();
    }
}

#endif // KEYBOARDLAYOUT_H
