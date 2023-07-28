/*
    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2022 Derek Christ <christ.derek@gmail.com>
    SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "trianglemousefilter.h"

#include <QPolygonF>
#include <cmath>

TriangleMouseFilter::TriangleMouseFilter(QQuickItem *parent)
    : QQuickItem(parent)
    , m_edgeLine()
    , m_active(true)
    , m_blockFirstEnter(false)
{
    setFiltersChildMouseEvents(true);

    m_resetTimer.setSingleShot(true);
    connect(&m_resetTimer, &QTimer::timeout, this, [this]() {
        m_lastCursorPosition.reset();
        m_lastTimestamp.reset();

        if (m_interceptedHoverItem) {
            resendHoverEvents(m_interceptedHoverItem.interceptedHoverEnterPosition.value());
        }

        m_interceptionPos.reset();
    });
};

bool TriangleMouseFilter::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    if (!m_active) {
        // Even if inactive, we still need to record the current item so when active becomes true after the child item is hovered, the filter can still work
        // correctly
        switch (event->type()) {
        case QEvent::HoverEnter:
            m_interceptedHoverItem = item;
            break;
        case QEvent::HoverLeave:
            m_interceptedHoverItem = nullptr;
            break;
        default:
            break;
        }
        return false;
    }

    switch (event->type()) {
    case QEvent::HoverLeave:
        if (m_interceptedHoverItem == item) {
            m_interceptedHoverItem = nullptr;
        }
        return false;
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        QHoverEvent &he = *static_cast<QHoverEvent *>(event);

        const QPointF position = item->mapToItem(this, he.posF());

        // This clause means that we block focus when first entering a given position
        // in the case of kickoff it's so that we can move the mouse from the bottom tabbar to the side view
        bool firstEnter = m_blockFirstEnter && event->type() == QEvent::HoverEnter && !m_interceptionPos;

        if (event->type() == QEvent::HoverMove && m_interceptedHoverItem == item && m_lastCursorPosition.has_value() && m_lastTimestamp.has_value()
            && !firstEnter) {
            // If no movement was registered, filter event in any case
            if (position == m_lastCursorPosition) {
                return true;
            }

            const QPointF deltaPosition = position - m_lastCursorPosition.value();
            m_lastCursorPosition = position;
            const auto deltaTime = he.timestamp() - m_lastTimestamp.value();
            m_lastTimestamp = he.timestamp();

            // As a first metric, we check the direction in which the cursor has been moved
            bool directionMetric = false;
            switch (m_edge) {
            case Qt::RightEdge:
                directionMetric = deltaPosition.x() < -JITTER_THRESHOLD;
                break;
            case Qt::TopEdge:
                directionMetric = deltaPosition.y() > JITTER_THRESHOLD;
                break;
            case Qt::LeftEdge:
                directionMetric = deltaPosition.x() > JITTER_THRESHOLD;
                break;
            case Qt::BottomEdge:
                directionMetric = deltaPosition.y() < -JITTER_THRESHOLD;
                break;
            }
            if (directionMetric) {
                resendHoverEvents(position);
                return true;
            }

            // As a second metric, we use the velocity of the cursor to disable the filter
            if (deltaTime != 0 && he.timestamp() != 0) {
                const double velocity = std::pow(deltaPosition.x(), 2) + std::pow(deltaPosition.y(), 2) / deltaTime;
                if (velocity < VELOCITY_THRESHOLD) {
                    resendHoverEvents(position);
                    return true;
                }
            }
        }

        // Finally, we check if the cursor movement was inside the filtered region
        if (firstEnter || filterContains(position)) {
            if (firstEnter) {
                // In case of a firstEnter, set the interceptionPos but not the interceptedHoverEnterPosition
                // so that the timer does not reselect the intercepted item
                m_interceptedHoverItem = item;
                m_interceptionPos = position;
            } else if (event->type() == QEvent::HoverEnter) {
                m_interceptedHoverItem = item;
                m_interceptedHoverItem.interceptedHoverEnterPosition = position;
            }

            m_lastCursorPosition = position;
            m_lastTimestamp = he.timestamp();

            if (m_filterTimeout > 0) {
                m_resetTimer.start(m_filterTimeout);
            }
            return true;
        } else {
            // Pass event through
            m_interceptionPos = position;

            if (he.type() == QEvent::HoverMove && m_interceptedHoverItem == item) {
                resendHoverEvents(position);
            }
            return false;
        }
    }
    default:
        return false;
    }
}

void TriangleMouseFilter::resendHoverEvents(const QPointF &cursorPosition)
{
    // If we are no longer inhibiting events and have previously intercepted a hover enter
    // we manually send the hover enter to that item
    if (m_interceptionPos) {
        const auto targetPosition = mapToItem(m_interceptedHoverItem.item, m_interceptionPos.value());
        QHoverEvent e(QEvent::HoverEnter, targetPosition, targetPosition);
        qApp->sendEvent(m_interceptedHoverItem.item, &e);
    }

    if (m_interceptionPos != cursorPosition) {
        const auto targetPosition = mapToItem(m_interceptedHoverItem.item, cursorPosition);
        QHoverEvent e(QEvent::HoverMove, targetPosition, targetPosition);
        qApp->sendEvent(m_interceptedHoverItem.item, &e);
    }

    m_interceptedHoverItem = nullptr;
}

bool TriangleMouseFilter::filterContains(const QPointF &p) const
{
    if (!m_interceptionPos) {
        return false;
    }

    // QPolygonF.contains returns false if we're on the edge, so we pad our main item
    const QRectF shape = (m_edgeLine.size() == 4) ? QRect(m_edgeLine[0] - 1, m_edgeLine[1] - 1, width() + m_edgeLine[2] + 1, height() + m_edgeLine[3] + 1)
                                                  : QRect(-1, -1, width() + 1, height() + 1);

    QPolygonF poly;

    // We use some jitter protection by extending our triangle out slight past the mouse position in the opposite direction of the edge;
    switch (m_edge) {
    case Qt::RightEdge:
        poly << m_interceptionPos.value() + QPointF(-JITTER_THRESHOLD, 0) << shape.topRight() << shape.bottomRight();
        break;
    case Qt::TopEdge:
        poly << m_interceptionPos.value() + QPointF(0, -JITTER_THRESHOLD) << shape.topLeft() << shape.topRight();
        break;
    case Qt::LeftEdge:
        poly << m_interceptionPos.value() + QPointF(JITTER_THRESHOLD, 0) << shape.topLeft() << shape.bottomLeft();
        break;
    case Qt::BottomEdge:
        poly << m_interceptionPos.value() + QPointF(0, JITTER_THRESHOLD) << shape.bottomLeft() << shape.bottomRight();
    }

    bool firstCheck = poly.containsPoint(p, Qt::OddEvenFill);
    poly.replace(0, m_secondaryPoint);
    bool secondCheck = m_secondaryPoint != QPointF(0, 0) && poly.containsPoint(p, Qt::OddEvenFill);
    return (firstCheck || secondCheck);
}
