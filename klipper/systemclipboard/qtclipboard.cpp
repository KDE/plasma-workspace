/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "qtclipboard.h"

#include <QClipboard>
#include <QGuiApplication>

QtClipboard::QtClipboard(QObject *parent)
    : SystemClipboard(parent)
{
    connect(qApp->clipboard(), &QClipboard::changed, this, &QtClipboard::changed);
}

void QtClipboard::setMimeData(QMimeData *mime, QClipboard::Mode mode)
{
    qApp->clipboard()->setMimeData(mime, mode);
}

void QtClipboard::clear(QClipboard::Mode mode)
{
    qApp->clipboard()->clear(mode);
}

const QMimeData *QtClipboard::mimeData(QClipboard::Mode mode) const
{
    return qApp->clipboard()->mimeData(mode);
}
