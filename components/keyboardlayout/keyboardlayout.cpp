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
#include "keyboard_layout_interface.h"

#include <QDBusInterface>

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

    mCurrentLayout = callDBus(mIface->getCurrentLayout());
    mLayouts = callDBus(mIface->getLayoutsList());
}

KeyboardLayout::~KeyboardLayout()
{
}

void KeyboardLayout::onCurrentLayoutChanged(const QString &newLayout)
{
    mCurrentLayout = newLayout;
    Q_EMIT currentLayoutChanged();
}

void KeyboardLayout::onLayoutListChanged()
{
    if (mIface) {
        mLayouts = callDBus(mIface->getLayoutsList());
        Q_EMIT layoutsChanged();
    }
}

QString KeyboardLayout::currentLayoutShortName() const
{
    return mIface ? callDBus(mIface->getCurrentLayoutShortName()) : QString();
}

QString KeyboardLayout::currentLayoutDisplayName() const
{
    return mIface ? callDBus(mIface->getLayoutDisplayName(mCurrentLayout)) : QString();
}

void KeyboardLayout::setCurrentLayout(const QString &layout)
{
    if (mIface) {
        mIface->setLayout(layout);
    }
}
