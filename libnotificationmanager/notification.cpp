/*
 * Copyright 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "notification.h"

#include <QDebug>
#include <QRegularExpression>

#include <KConfig>
#include <KService>

using namespace NotificationManager;

Notification::Notification(uint id)
    : m_id(id)
    , m_created(QDateTime::currentDateTimeUtc())
{
    // QVector needs default constructor
    //Q_ASSERT(id > 0);
}

Notification::~Notification() = default;

uint Notification::id() const
{
    return m_id;
}

QDateTime Notification::created() const
{
    return m_created;
}

QString Notification::summary() const
{
    return m_summary;
}

void Notification::setSummary(const QString &summary)
{
    m_summary = summary;
}

QString Notification::body() const
{
    return m_body;
}

void Notification::setBody(const QString &body)
{
    // FIXME FIXME FIXME Sanitize
    // should we sanitize depending on configured server capabilities?
    m_body = body;
}

QString Notification::iconName() const
{
    return m_iconName;
}

void Notification::setIconName(const QString &iconName)
{
    m_iconName = iconName;
}

QImage Notification::image() const
{
    return m_image;
}

void Notification::setImage(const QImage &image)
{
    m_image = image;
}

QString Notification::applicationName() const
{
    return m_applicationName;
}

void Notification::setApplicationName(const QString &applicationName)
{
    m_applicationIconName = applicationName;
}

QString Notification::applicationIconName() const
{
    return m_applicationIconName;
}

void Notification::setApplicationIconName(const QString &applicationIconName)
{
    m_applicationIconName = applicationIconName;
}

QStringList Notification::actionNames() const
{
    return m_actionNames;
}

QStringList Notification::actionLabels() const
{
    return m_actionLabels;
}

bool Notification::hasDefaultAction() const
{
    return m_hasDefaultAction;
}

/*bool Notification::hasConfigureAction() const
{
    return m_hasConfigureAction;
}*/

void Notification::setActions(const QStringList &actions)
{
    if (actions.count() % 2 != 0) {
        // FIXME qCWarning
        qWarning() << "List of actions must contain an even number of items, tried to set actions to" << actions;
        return;
    }

    QStringList names;
    QStringList labels;

    for (int i = 0; i < actions.count(); i += 2) {
        const QString &name = actions.at(i);
        const QString &label = actions.at(i + 1);

        if (!m_hasDefaultAction && name == QLatin1String("default")) {
            m_hasDefaultAction = true;
            continue;
        }

        if (!m_hasConfigureAction && name == QLatin1String("settings")) {
            m_configurable = true;
            m_hasConfigureAction = true;
            m_configureActionLabel = label;
            continue;
        }

        names << name;
        labels << label;
    }
    qDebug() << "setact" << actions;

    m_actionNames = names;
    m_actionLabels = labels;
}

QList<QUrl> Notification::urls() const
{
    return m_urls;
}

void Notification::setUrls(const QList<QUrl> &urls)
{
    m_urls = urls;
}

Notification::Urgency Notification::urgency() const
{
    return m_urgency;
}

void Notification::setUrgency(Notification::Urgency urgency)
{
    m_urgency = urgency;
}

int Notification::timeout() const
{
    return m_timeout;
}

void Notification::setTimeout(int timeout)
{
    if (timeout == -1) {
        // FIXME server default
        // old impl use some heuristic based on text length
        timeout = 5000;
    }

    m_timeout = timeout;
}

// what about "resident" hint?
bool Notification::isPersistent() const
{
    return m_timeout == 0;
}

bool Notification::isConfigurable() const
{
    return m_configurable;
}

QString Notification::configureActionLabel() const
{
    return m_configureActionLabel;
}

bool Notification::expired() const
{
    return m_expired;
}

void Notification::setExpired(bool expired)
{
    m_expired = expired;
}

bool Notification::seen() const
{
    return m_seen;
}

void Notification::setSeen(bool seen)
{
    m_seen = seen;
}

void Notification::processHints(const QVariantMap &hints)
{
    const QString desktopEntry = hints.value(QStringLiteral("desktop-entry")).toString();
    if (!desktopEntry.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(desktopEntry);
        if (service) {
            m_applicationName = service->name();
            m_applicationIconName = service->icon();
        }
    }

    m_notifyRcName = hints.value(QStringLiteral("x-kde-appname")).toString();
    if (!m_notifyRcName.isEmpty()) {
        //const QString eventId = hints.value(QStringLiteral("x-kde-eventId")).toString();
        // TODO

        // Check whether the application actually has notifications we can configure
        // FIXME move out into a separate function
        KConfig config(m_notifyRcName + QStringLiteral(".notifyrc"), KConfig::NoGlobals);
        config.addConfigSources(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                QStringLiteral("knotifications5/") + m_notifyRcName + QStringLiteral(".notifyrc")));

        const QRegularExpression regexp(QStringLiteral("^Event/([^/]*)$"));

        const bool configurable = !config.groupList().filter(regexp).isEmpty();
        m_configurable |= configurable;
    }

    m_eventId = hints.value(QStringLiteral("x-kde-eventId")).toString();

    // TODO x-kde-skipGrouping

    // TODO image-data/image_data/image-path/image_path
}

bool Notification::operator==(const Notification &other) const
{
    return other.m_id == m_id;
}
