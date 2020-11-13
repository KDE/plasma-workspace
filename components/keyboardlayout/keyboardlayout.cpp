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
void KeyboardLayout::requestDBusData<KeyboardLayout::CurrentLayoutShortName>()
{ if (mIface) requestDBusData(mIface->getCurrentLayoutShortName(), mCurrentLayoutShortName, &KeyboardLayout::currentLayoutShortNameChanged); }

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
            this, &KeyboardLayout::onCurrentLayoutChanged);
    connect(mIface, &OrgKdeKeyboardLayoutsInterface::layoutListChanged,
            this, &KeyboardLayout::onLayoutListChanged);
    connect(this, &KeyboardLayout::currentLayoutChanged,
            this, &KeyboardLayout::requestDBusData<CurrentLayoutDisplayName>);

    requestDBusData<CurrentLayout>();
    requestDBusData<CurrentLayoutShortName>();
    requestDBusData<Layouts>();
}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::onCurrentLayoutChanged(const QString &newLayout)
{
    mCurrentLayout = newLayout;

    requestDBusData<CurrentLayoutShortName>();
    requestDBusData<CurrentLayoutDisplayName>();
}

void KeyboardLayout::onLayoutListChanged()
{
    requestDBusData<CurrentLayout>();
    requestDBusData<CurrentLayoutShortName>();
    requestDBusData<Layouts>();
}

void KeyboardLayout::setCurrentLayout(const QString &layout)
{
    if (mIface) mIface->setLayout(layout);
}
