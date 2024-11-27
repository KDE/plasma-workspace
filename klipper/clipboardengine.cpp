/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipboardengine.h"
#include "clipboardservice.h"
#include "historycycler.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipper.h"

#include <QCoreApplication>

static const QString s_clipboardSourceName = QStringLiteral("clipboard");
static const QString s_barcodeKey = QStringLiteral("supportsBarcodes");

ClipboardEngine::ClipboardEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
    , m_klipper(Klipper::self())
    , m_model(HistoryModel::self())
{
    // TODO: use a filterproxymodel
    setModel(s_clipboardSourceName, m_model.get());
    // Unset parent to avoid double delete as DataEngine::setModel will set parent for the model
    m_model.get()->setParent(nullptr);
    setData(s_clipboardSourceName, s_barcodeKey, true);
    auto updateCurrent = [this](bool isTop = true) {
        if (isTop) {
            setData(s_clipboardSourceName, QStringLiteral("current"), m_model->rowCount() == 0 ? QString() : m_model->first()->text());
        }
    };
    connect(m_model.get(), &HistoryModel::changed, this, updateCurrent);
    updateCurrent();
    auto updateEmpty = [this]() {
        setData(s_clipboardSourceName, QStringLiteral("empty"), m_model->rowCount() == 0);
    };
    connect(m_model.get(), &HistoryModel::changed, this, updateEmpty);
    updateEmpty();
}

ClipboardEngine::~ClipboardEngine()
{
    if (!QCoreApplication::closingDown()) {
        m_klipper->saveClipboardHistory();
    }
}

Plasma5Support::Service *ClipboardEngine::serviceForSource(const QString &source)
{
    Plasma5Support::Service *service = new ClipboardService(m_klipper.get(), source);
    service->setParent(this);
    return service;
}

K_PLUGIN_CLASS_WITH_JSON(ClipboardEngine, "plasma-dataengine-clipboard.json")

#include "clipboardengine.moc"

#include "moc_clipboardengine.cpp"
