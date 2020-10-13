/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "clipboardengine.h"
#include "clipboardservice.h"
#include "history.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipper.h"

static const QString s_clipboardSourceName = QStringLiteral("clipboard");
static const QString s_barcodeKey = QStringLiteral("supportsBarcodes");

ClipboardEngine::ClipboardEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
    , m_klipper(new Klipper(this, KSharedConfig::openConfig(QStringLiteral("klipperrc")), KlipperMode::DataEngine))
{
    // TODO: use a filterproxymodel
    setModel(s_clipboardSourceName, m_klipper->history()->model());
    setData(s_clipboardSourceName, s_barcodeKey, true);
    auto updateCurrent = [this]() {
        setData(s_clipboardSourceName,
                QStringLiteral("current"),
                m_klipper->history()->empty() ? QString() : m_klipper->history()->first()->text());
    };
    connect(m_klipper->history(), &History::topChanged, this, updateCurrent);
    updateCurrent();
    auto updateEmpty = [this]() {
        setData(s_clipboardSourceName, QStringLiteral("empty"), m_klipper->history()->empty());
    };
    connect(m_klipper->history(), &History::changed, this, updateEmpty);
    updateEmpty();
}

ClipboardEngine::~ClipboardEngine()
{
    m_klipper->saveClipboardHistory();
}

Plasma::Service *ClipboardEngine::serviceForSource(const QString &source)
{
    Plasma::Service *service = new ClipboardService(m_klipper, source);
    service->setParent(this);
    return service;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(org.kde.plasma.clipboard, ClipboardEngine, "plasma-dataengine-clipboard.json")

#include "clipboardengine.moc"
