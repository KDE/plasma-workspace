/*
 * Copyright (C) 2014  Daniel Vratil <dvratil@redhat.com>
 * Copyright (C) 2019  David Edmundson <davidedmundson@kde.org>
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

#include <QDBusInterface>
#include <QDBusReply>

#include <QDebug>
#include "debug.h"

#include "keyboard_layout_interface.h"

KeyboardLayout::KeyboardLayout(QObject* parent)
    : QObject(parent)
    , mIface(nullptr)
{
    mIface = new OrgKdeKeyboardLayoutsInterface(QStringLiteral("org.kde.keyboard"),
                                QStringLiteral("/Layouts"),
                                QDBusConnection::sessionBus(),
                                this);
    if (!mIface->isValid()) {
          delete mIface;
          mIface = nullptr;
          return;
    }

    connect(mIface, &OrgKdeKeyboardLayoutsInterface::currentLayoutChanged,
            this, &KeyboardLayout::setCurrentLayout);
    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutListChanged,
            this, &KeyboardLayout::requestLayoutsList);

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

    QDBusPendingReply<QString> pendingLayout = mIface->getCurrentLayout();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingLayout, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &KeyboardLayout::onCurrentLayoutReceived);
}

void KeyboardLayout::onCurrentLayoutReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
    } else {
        mCurrentLayout = reply.value();
        requestCurrentLayoutDisplayName();
        Q_EMIT currentLayoutChanged(mCurrentLayout);
    }
    watcher->deleteLater();
}

void KeyboardLayout::requestCurrentLayoutDisplayName()
{
    QDBusPendingReply<QString> pendingDisplayName = mIface->getLayoutDisplayName(mCurrentLayout);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingDisplayName, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &KeyboardLayout::onCurrentLayoutDisplayNameReceived);
}

void KeyboardLayout::onCurrentLayoutDisplayNameReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
    } else {
        mCurrentLayoutDisplayName = reply.value();
        Q_EMIT currentLayoutDisplayNameChanged(mCurrentLayoutDisplayName);
    }
    watcher->deleteLater();
}

QString KeyboardLayout::currentLayoutDisplayName() const
{
    return mCurrentLayoutDisplayName;
}

void KeyboardLayout::requestLayoutsList()
{
    if (!mIface) {
        return;
    }

    QDBusPendingReply<QStringList> pendingLayout = mIface->getLayoutsList();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingLayout, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &KeyboardLayout::onLayoutsListReceived);
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
    requestCurrentLayoutDisplayName();
    mIface->setLayout(layout);
    Q_EMIT currentLayoutChanged(layout);
}


QStringList KeyboardLayout::layouts() const
{
    return mLayouts;
}

