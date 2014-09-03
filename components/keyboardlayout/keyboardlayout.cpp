/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "keyboardlayout.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <QDebug>

Q_LOGGING_CATEGORY(KEYBOARD_LAYOUT, "org.kde.keyboardLayout")

KeyboardLayout::KeyboardLayout(QObject* parent)
    : QObject(parent)
    , mIface(0)
{
    mIface = new QDBusInterface(QLatin1String("org.kde.kded5"),
                                QLatin1String("/modules/keyboard"),
                                QLatin1String("org.kde.KeyboardLayouts"),
                                QDBusConnection::sessionBus(),
                                this);
    if (!mIface->isValid()) {
          delete mIface;
          mIface = 0;
          return;
    }

    connect(mIface, SIGNAL(currentLayoutChanged(QString)),
            this, SLOT(setCurrentLayout(QString)));
    connect(mIface, SIGNAL(layoutListChanged()),
            this, SLOT(requestLayoutsList()));

    requestCurrentLayout();
    requestLayoutsList();

}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::requestCurrentLayout()
{
    if (!mIface) {
        return;
    }

    QDBusPendingCall pendingLayout = mIface->asyncCall(QLatin1String("getCurrentLayout"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingLayout, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(onCurrentLayoutReceived(QDBusPendingCallWatcher*)));
}

void KeyboardLayout::onCurrentLayoutReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
    } else {
        mCurrentLayout = reply.value();
        Q_EMIT currentLayoutChanged(mCurrentLayout);
    }
    watcher->deleteLater();
}

void KeyboardLayout::requestLayoutsList()
{
    if (!mIface) {
        return;
    }

    QDBusPendingCall pendingLayout = mIface->asyncCall(QLatin1String("getLayoutsList"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingLayout, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(onLayoutsListReceived(QDBusPendingCallWatcher*)));
}


void KeyboardLayout::onLayoutsListReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QStringList> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
    } else {
        mLayouts = reply.value();
        qCDebug(KEYBOARD_LAYOUT) << "Layouts list changed: " << mLayouts;
        Q_EMIT layoutsChanged();
    }
    watcher->deleteLater();
}

QString KeyboardLayout::currentLayout() const
{
    return mCurrentLayout;
}

void KeyboardLayout::setCurrentLayout(const QString &layout)
{
    if (!mIface) {
        return;
    }
    if (mCurrentLayout == layout) {
        return;
    }

    if (!mLayouts.contains(layout)) {
        qCWarning(KEYBOARD_LAYOUT) << "No such layout" << layout;
        return;
    }

    mCurrentLayout = layout;
    mIface->asyncCall(QLatin1String("setLayout"), layout);
    Q_EMIT currentLayoutChanged(layout);
}


QStringList KeyboardLayout::layouts() const
{
    return mLayouts;
}

void KeyboardLayout::onCurrentLayoutChanged(const QString &newLayout)
{
    mCurrentLayout = newLayout;
    Q_EMIT currentLayoutChanged(newLayout);
}

