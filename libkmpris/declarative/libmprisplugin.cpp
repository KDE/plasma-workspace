/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "libmprisplugin.h"

#include <QQmlEngine>

#include "mpris2model.h"
#include "multiplexermodel.h"
#include "playercontainer.h"

void MPRISPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<Mpris2Model>(uri, 0, 1, "Mpris2Model");
    qmlRegisterType<MultiplexerModel>(uri, 0, 1, "MultiplexerModel");

    qmlRegisterAnonymousType<PlayerContainer>(uri, 0);

    const QString errorMessage = QStringLiteral("Error: only enums");
    qmlRegisterUncreatableMetaObject(LoopStatus::staticMetaObject, uri, 0, 1, "LoopStatus", errorMessage);
    qmlRegisterUncreatableMetaObject(ShuffleStatus::staticMetaObject, uri, 0, 1, "ShuffleStatus", errorMessage);
    qmlRegisterUncreatableMetaObject(PlaybackStatus::staticMetaObject, uri, 0, 1, "PlaybackStatus", errorMessage);
}
