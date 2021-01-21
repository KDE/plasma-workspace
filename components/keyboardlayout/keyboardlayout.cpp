/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "keyboardlayout.h"
#include "keyboard_layout_interface.h"

#include <QDBusInterface>

template<> inline void KeyboardLayout::requestDBusData<KeyboardLayout::Layout>()
{
    if (mIface)
        requestDBusData(mIface->getLayout(), mLayout, &KeyboardLayout::layoutChanged);
}

template<> inline void KeyboardLayout::requestDBusData<KeyboardLayout::LayoutsList>()
{
    if (mIface)
        requestDBusData(mIface->getLayoutsList(), mLayoutsList, &KeyboardLayout::layoutsListChanged);
}

KeyboardLayout::KeyboardLayout(QObject *parent)
    : QObject(parent)
    , mIface(nullptr)
{
    LayoutNames::registerMetaType();

    mIface = new OrgKdeKeyboardLayoutsInterface(QStringLiteral("org.kde.keyboard"), QStringLiteral("/Layouts"), QDBusConnection::sessionBus(), this);
    if (!mIface->isValid()) {
        delete mIface;
        mIface = nullptr;
        return;
    }

    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutChanged, this, [this](uint index) {
        mLayout = index;
        emit layoutChanged();
    });

    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutListChanged, this, [this]() {
        requestDBusData<LayoutsList>();
        requestDBusData<Layout>();
    });

    emit mIface->OrgKdeKeyboardLayoutsInterface::layoutListChanged();
}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::switchToNextLayout()
{
    if (mIface)
        mIface->switchToNextLayout();
}

void KeyboardLayout::switchToPreviousLayout()
{
    if (mIface)
        mIface->switchToPreviousLayout();
}

void KeyboardLayout::setLayout(uint index)
{
    if (mIface)
        mIface->setLayout(index);
}

template<class T> void KeyboardLayout::requestDBusData(QDBusPendingReply<T> pendingReply, T &out, void (KeyboardLayout::*notify)())
{
    connect(new QDBusPendingCallWatcher(pendingReply, this), &QDBusPendingCallWatcher::finished, this, [this, &out, notify](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<T> reply = *watcher;
        if (reply.isError()) {
            qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
        }
        out = reply.value();
        emit(this->*notify)();

        watcher->deleteLater();
    });
}
