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
void KeyboardLayout::requestDBusData<KeyboardLayout::CurrentLayout>()
{ if (mIface) requestDBusData(mIface->getCurrentLayout(), mCurrentLayout, &KeyboardLayout::currentLayoutChanged); }

template<>
void KeyboardLayout::requestDBusData<KeyboardLayout::CurrentLayoutDisplayName>()
{ if (mIface) requestDBusData(mIface->getLayoutDisplayName(mCurrentLayout), mCurrentLayoutDisplayName, &KeyboardLayout::currentLayoutDisplayNameChanged); }

template<>
void KeyboardLayout::requestDBusData<KeyboardLayout::CurrentLayoutLongName>()
{ if (mIface) requestDBusData(mIface->getCurrentLayoutLongName(), mCurrentLayoutLongName, &KeyboardLayout::currentLayoutLongNameChanged); }

template<>
void KeyboardLayout::requestDBusData<KeyboardLayout::Layouts>()
{ if (mIface) requestDBusData(mIface->getLayoutsList(), mLayouts, &KeyboardLayout::layoutsChanged); }


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
            this, [this](const QString &newLayout)
                    {
                        mCurrentLayout = newLayout;

                        requestDBusData<CurrentLayoutDisplayName>();
                        requestDBusData<CurrentLayoutLongName>();
                    });
    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutListChanged,
            this, [this]()
                    {
                        requestDBusData<CurrentLayout>();
                        requestDBusData<CurrentLayoutLongName>();
                        requestDBusData<Layouts>();
                    });
    connect(this, &KeyboardLayout::currentLayoutChanged,
            this, &KeyboardLayout::requestDBusData<CurrentLayoutDisplayName>);

    emit mIface->OrgKdeKeyboardLayoutsInterface::layoutListChanged();
}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::switchToNextLayout()
{
    if (mIface) mIface->switchToNextLayout();
}

void KeyboardLayout::setCurrentLayout(const QString &layout)
{
    if (mIface) mIface->setLayout(layout);
}

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
