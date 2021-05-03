/*
 * Copyright 2008 Alain Boyer <alainboyer@gmail.com>
 * Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "statusnotifieritemjob.h"
#include <iostream>

StatusNotifierItemJob::StatusNotifierItemJob(StatusNotifierItemSource *source, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent)
    : ServiceJob(source->objectName(), operation, parameters, parent)
    , m_source(source)
{
    // Queue connection, so that all 'deleteLater' are performed before we use updated menu.
    connect(source, SIGNAL(contextMenuReady(QMenu *)), this, SLOT(contextMenuReady(QMenu *)), Qt::QueuedConnection);
    connect(source, &StatusNotifierItemSource::activateResult, this, &StatusNotifierItemJob::activateCallback);
}

StatusNotifierItemJob::~StatusNotifierItemJob()
{
}

void StatusNotifierItemJob::start()
{
    if (operationName() == QString::fromLatin1("Activate")) {
        m_source->activate(parameters()[QStringLiteral("x")].toInt(), parameters()[QStringLiteral("y")].toInt());
    } else if (operationName() == QString::fromLatin1("SecondaryActivate")) {
        m_source->secondaryActivate(parameters()[QStringLiteral("x")].toInt(), parameters()[QStringLiteral("y")].toInt());
        setResult(0);
    } else if (operationName() == QString::fromLatin1("ContextMenu")) {
        m_source->contextMenu(parameters()[QStringLiteral("x")].toInt(), parameters()[QStringLiteral("y")].toInt());
    } else if (operationName() == QString::fromLatin1("Scroll")) {
        m_source->scroll(parameters()[QStringLiteral("delta")].toInt(), parameters()[QStringLiteral("direction")].toString());
        setResult(0);
    }
}

void StatusNotifierItemJob::activateCallback(bool success)
{
    if (operationName() == QString::fromLatin1("Activate")) {
        setResult(QVariant(success));
    }
}

void StatusNotifierItemJob::contextMenuReady(QMenu *menu)
{
    if (operationName() == QString::fromLatin1("ContextMenu")) {
        setResult(QVariant::fromValue((QObject *)menu));
    }
}
