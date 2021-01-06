/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 * SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
 * SPDX-FileCopyrightText: 2020 Andrey Butirsky <butirsky@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "keyboardlayout.h"
#include "keyboard_layout_interface.h"

#include <QDBusInterface>

template<>
inline void KeyboardLayout::requestDBusData<KeyboardLayout::Layout>()
{ if (mIface) requestDBusData(mIface->getLayout(), &KeyboardLayout::layoutChanged); }

template<>
inline void KeyboardLayout::requestDBusData<KeyboardLayout::Layouts>()
{ if (mIface) requestDBusData(mIface->getLayoutsList(), &KeyboardLayout::layoutsChanged); }

template<>
inline void KeyboardLayout::requestDBusData<KeyboardLayout::LayoutLongName>()
{ if (mIface) requestDBusData(mIface->getLayoutLongName(), &KeyboardLayout::layoutLongNameChanged); }

void KeyboardLayout::requestLayoutLongName()
{
    requestDBusData<LayoutLongName>();
}


KeyboardLayout::KeyboardLayout(QObject* parent)
    : QObject(parent)
    , mIface(nullptr)
{
    LayoutNames::registerMetaType();

    mIface = new OrgKdeKeyboardLayoutsInterface(QStringLiteral("org.kde.keyboard"),
                                QStringLiteral("/Layouts"),
                                QDBusConnection::sessionBus(),
                                this);
    if (!mIface->isValid()) {
          delete mIface;
          mIface = nullptr;
          return;
    }

    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutChanged,
            this, &KeyboardLayout::layoutChanged);

    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutListChanged,
            this, [this]()
                    {
                        requestDBusData<Layouts>();
                        requestDBusData<Layout>();
                    });

    emit mIface->OrgKdeKeyboardLayoutsInterface::layoutListChanged();
}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::switchToNextLayout()
{
    if (mIface) mIface->switchToNextLayout();
}

void KeyboardLayout::switchToPreviousLayout()
{
    if (mIface) mIface->switchToPreviousLayout();
}

void KeyboardLayout::setLayout(const QString &layout)
{
    if (mIface) mIface->setLayout(layout);
}

template<class T>
void KeyboardLayout::requestDBusData(QDBusPendingReply<T> pendingReply, void (KeyboardLayout::*notify)(const T &))
{
    const QDBusPendingCallWatcher * const watcher = new QDBusPendingCallWatcher(pendingReply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
        [this, notify](QDBusPendingCallWatcher *watcher)
        {
            QDBusPendingReply<T> reply = *watcher;
            if (reply.isError()) {
                qCWarning(KEYBOARD_LAYOUT) << reply.error().message();
            } else {
                emit (this->*notify)(reply.value());
            }
            watcher->deleteLater();
        }
    );
}
