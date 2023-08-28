/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipboardengine.h"
#include "clipboardservice.h"
#include "history.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipper.h"

static const QString s_clipboardSourceName = QStringLiteral("clipboard");
static const QString s_barcodeKey = QStringLiteral("supportsBarcodes");

ClipboardEngine::ClipboardEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
    , m_klipper(new Klipper(this, KSharedConfig::openConfig(QStringLiteral("klipperrc")), KlipperMode::DataEngine))
{
    // TODO: use a filterproxymodel
    setModel(s_clipboardSourceName, m_klipper->history()->model());
    setData(s_clipboardSourceName, s_barcodeKey, true);
    auto updateCurrent = [this]() {
        setData(s_clipboardSourceName, QStringLiteral("current"), m_klipper->history()->empty() ? QString() : m_klipper->history()->first()->text());
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

Plasma5Support::Service *ClipboardEngine::serviceForSource(const QString &source)
{
    Plasma5Support::Service *service = new ClipboardService(m_klipper, source);
    service->setParent(this);
    return service;
}

K_PLUGIN_CLASS_WITH_JSON(ClipboardEngine, "plasma-dataengine-clipboard.json")

#include "clipboardengine.moc"
