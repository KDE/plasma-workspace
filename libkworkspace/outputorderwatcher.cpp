/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outputorderwatcher.h"

#include <version>

#include <QGuiApplication>

#include "qwayland-kde-output-order-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/QtWaylandClientVersion>

class OutputOrderWatcherPrivate : public QWaylandClientExtensionTemplate<OutputOrderWatcherPrivate, &QtWayland::kde_output_order_v1::destroy>,
                                  public QtWayland::kde_output_order_v1
{
    Q_OBJECT
public:
    OutputOrderWatcherPrivate(OutputOrderWatcher *_q)
        : QWaylandClientExtensionTemplate(1)
        , q(_q)
    {
        initialize();
    }

    QStringList m_outputOrder;

protected:
    void kde_output_order_v1_output(const QString &outputName) override
    {
        if (m_done) {
            m_pendingOutputOrder.clear();
            m_done = false;
        }
        m_pendingOutputOrder.append(outputName);
    }

    void kde_output_order_v1_done() override
    {
        if (m_done) {
            m_pendingOutputOrder.clear();
        }
        m_done = true;
        if (m_outputOrder != m_pendingOutputOrder) {
            m_outputOrder = m_pendingOutputOrder;
            Q_EMIT q->outputOrderChanged(m_outputOrder);
        }
    }

private:
    QStringList m_pendingOutputOrder;
    bool m_done = true;
    OutputOrderWatcher *q;
};

OutputOrderWatcher::OutputOrderWatcher(QObject *parent)
    : QObject(parent)
    , d(new OutputOrderWatcherPrivate(this))
{
}

OutputOrderWatcher::~OutputOrderWatcher()
{
}

QStringList OutputOrderWatcher::outputOrder() const
{
    return d->m_outputOrder;
}

#include "outputorderwatcher.moc"

#include "moc_outputorderwatcher.cpp"
