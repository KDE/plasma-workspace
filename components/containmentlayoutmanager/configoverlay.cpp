/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "configoverlay.h"

#include <cmath>

ConfigOverlay::ConfigOverlay(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(600);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        setVisible(false);
    });
}

ConfigOverlay::~ConfigOverlay()
{
}

bool ConfigOverlay::open() const
{
    return m_open;
}

void ConfigOverlay::setOpen(bool open)
{
    if (open == m_open) {
        return;
    }

    m_open = open;

    if (open) {
        m_hideTimer->stop();
        setVisible(true);
    } else {
        m_hideTimer->start();
    }

    emit openChanged();
}

bool ConfigOverlay::touchInteraction() const
{
    return m_touchInteraction;
}
void ConfigOverlay::setTouchInteraction(bool touch)
{
    if (touch == m_touchInteraction) {
        return;
    }

    m_touchInteraction = touch;
    emit touchInteractionChanged();
}

ItemContainer *ConfigOverlay::itemContainer() const
{
    return m_itemContainer;
}

void ConfigOverlay::setItemContainer(ItemContainer *container)
{
    if (container == m_itemContainer) {
        return;
    }

    if (m_itemContainer) {
        disconnect(m_itemContainer, nullptr, this, nullptr);
    }

    m_itemContainer = container;

    if (!m_itemContainer || !m_itemContainer->layout()) {
        return;
    }

    m_leftAvailableSpace = qMax(0.0, m_itemContainer->x());
    m_rightAvailableSpace = qMax(0.0, m_itemContainer->layout()->width() - (m_itemContainer->x() + m_itemContainer->width()));
    m_topAvailableSpace = qMax(0.0, m_itemContainer->y());
    m_bottomAvailableSpace = qMax(0.0, m_itemContainer->layout()->height() - (m_itemContainer->y() + m_itemContainer->height()));
    emit leftAvailableSpaceChanged();
    emit rightAvailableSpaceChanged();
    emit topAvailableSpaceChanged();
    emit bottomAvailableSpaceChanged();

    connect(m_itemContainer.data(), &ItemContainer::xChanged, this, [this]() {
        m_leftAvailableSpace = qMax(0.0, m_itemContainer->x());
        m_rightAvailableSpace = qMax(0.0, m_itemContainer->layout()->width() - (m_itemContainer->x() + m_itemContainer->width()));
        emit leftAvailableSpaceChanged();
        emit rightAvailableSpaceChanged();
    });

    connect(m_itemContainer.data(), &ItemContainer::yChanged, this, [this]() {
        m_topAvailableSpace = qMax(0.0, m_itemContainer->y());
        m_bottomAvailableSpace = qMax(0.0, m_itemContainer->layout()->height() - (m_itemContainer->y() + m_itemContainer->height()));
        emit topAvailableSpaceChanged();
        emit bottomAvailableSpaceChanged();
    });

    connect(m_itemContainer.data(), &ItemContainer::widthChanged, this, [this]() {
        m_rightAvailableSpace = qMax(0.0, m_itemContainer->layout()->width() - (m_itemContainer->x() + m_itemContainer->width()));
        emit rightAvailableSpaceChanged();
    });

    connect(m_itemContainer.data(), &ItemContainer::heightChanged, this, [this]() {
        m_bottomAvailableSpace = qMax(0.0, m_itemContainer->layout()->height() - (m_itemContainer->y() + m_itemContainer->height()));
        emit bottomAvailableSpaceChanged();
    });
    emit itemContainerChanged();
}

#include "moc_configoverlay.cpp"
