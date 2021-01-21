/*
 *   Copyright (C) 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "geolocationprovider.h"

GeolocationProvider::GeolocationProvider(QObject *parent, const QVariantList &args)
    : QObject(parent)
    , m_sharedData(nullptr)
    , m_sharedAccuracies(nullptr)
    , m_accuracy(1000)
    , m_updateTriggers(SourceEvent)
    , m_available(true)
    , m_updating(false)
{
    Q_UNUSED(args)
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    qRegisterMetaType<Plasma::DataEngine::Data>("Plasma::DataEngine::Data");
    connect(&m_updateTimer, &QTimer::timeout, this, &GeolocationProvider::updated);
}

void GeolocationProvider::init(Plasma::DataEngine::Data *data, EntryAccuracy *accuracies)
{
    m_sharedData = data;
    m_sharedAccuracies = accuracies;
    init();
}

int GeolocationProvider::accuracy() const
{
    return m_accuracy;
}

bool GeolocationProvider::isAvailable() const
{
    return m_available;
}

bool GeolocationProvider::requestUpdate(GeolocationProvider::UpdateTriggers triggers)
{
    if (m_available && !m_updating && (triggers == ForcedUpdate || triggers & m_updateTriggers)) {
        m_updating = true;
        update();
        return true;
    }

    return false;
}

GeolocationProvider::UpdateTriggers GeolocationProvider::updateTriggers() const
{
    return m_updateTriggers;
}

bool GeolocationProvider::populateSharedData()
{
    Plasma::DataEngine::Data::const_iterator it = m_data.constBegin();
    bool changed = false;

    while (it != m_data.constEnd()) {
        if (!m_sharedData->contains(it.key()) || m_sharedAccuracies->value(it.key()) < m_accuracy) {
            m_sharedData->insert(it.key(), it.value());
            m_sharedAccuracies->insert(it.key(), m_accuracy);
            changed = true;
        }

        ++it;
    }

    return changed;
}

void GeolocationProvider::setAccuracy(int accuracy)
{
    m_accuracy = accuracy;
}

void GeolocationProvider::setIsAvailable(bool available)
{
    if (m_available == available) {
        return;
    }

    m_available = available;
    Q_EMIT availabilityChanged(this);
}

void GeolocationProvider::setData(const Plasma::DataEngine::Data &data)
{
    m_updating = false;
    m_data = data;
    if (populateSharedData()) {
        m_updateTimer.start();
    }
}

void GeolocationProvider::setData(const QString &key, const QVariant &value)
{
    m_updating = false;
    m_data.insert(key, value);

    if (!m_sharedData->contains(key) || m_sharedAccuracies->value(QStringLiteral("key")) < m_accuracy) {
        m_sharedData->insert(key, value);
        m_sharedAccuracies->insert(key, accuracy());
        m_updateTimer.start();
    }
}

void GeolocationProvider::setUpdateTriggers(UpdateTriggers triggers)
{
    m_updateTriggers = triggers;
}

void GeolocationProvider::init()
{
}

void GeolocationProvider::update()
{
}
