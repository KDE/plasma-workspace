/***************************************************************************
 *   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>                  *
 *   Copyright (C) 2020 Méven Car <meven.car@kdemail.net>                  *
 *   Copyright (C) 2020 Tobias Fella <fella@posteo.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#include "componentchooser.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KOpenWithDialog>
#include <KQuickAddons/ConfigModule>
#include <KService>
#include <KSharedConfig>

ComponentChooser::ComponentChooser(QObject *parent, const QString &mimeType, const QString &type, const QString &defaultApplication, const QString &dialogText)
    : QObject(parent)
    , m_mimeType(mimeType)
    , m_type(type)
    , m_defaultApplication(defaultApplication)
    , m_dialogText(dialogText)
{
}

void ComponentChooser::defaults()
{
    if (m_defaultIndex) {
        select(*m_defaultIndex);
    }
}

void ComponentChooser::load()
{
    m_applications.clear();

    bool preferredServiceAdded = false;

    KService::Ptr preferredService = KApplicationTrader::preferredService(m_mimeType);

    KApplicationTrader::query([&preferredServiceAdded, preferredService, this](const KService::Ptr &service) {
        if (service->exec().isEmpty() || !service->categories().contains(m_type) || (!service->serviceTypes().contains(m_mimeType))) {
            return false;
        }
        QVariantMap application;
        application["name"] = service->name();
        application["icon"] = service->icon();
        application["storageId"] = service->storageId();
        m_applications += application;
        if ((preferredService && preferredService->storageId() == service->storageId())) {
            m_index = m_applications.length() - 1;
            preferredServiceAdded = true;
        }
        if (service->storageId() == m_defaultApplication) {
            m_defaultIndex = m_applications.length() - 1;
        }
        return false;
    });
    if (preferredService && !preferredServiceAdded) {
        // standard application was specified by the user
        QVariantMap application;
        application["name"] = preferredService->name();
        application["icon"] = preferredService->icon();
        application["storageId"] = preferredService->storageId();
        m_applications += application;
        m_index = m_applications.length() - 1;
    }
    QVariantMap application;
    application["name"] = i18n("Other...");
    application["icon"] = QStringLiteral("application-x-shellscript");
    application["storageId"] = QLatin1String("");
    m_applications += application;
    if (m_index == -1) {
        m_index = 0;
    }

    m_previousApplication = m_applications[m_index].toMap()["storageId"].toString();
    Q_EMIT applicationsChanged();
    Q_EMIT indexChanged();
    Q_EMIT isDefaultsChanged();
}

void ComponentChooser::select(int index)
{
    if (m_index == index && m_applications.size() != 1) {
        return;
    }
    if (index == m_applications.length() - 1) {
        KOpenWithDialog *dialog = new KOpenWithDialog(QList<QUrl>(), m_mimeType, m_dialogText, QString());
        dialog->setSaveNewApplications(true);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, &KOpenWithDialog::finished, this, [this, dialog](int result) {
            if (result == QDialog::Rejected) {
                Q_EMIT indexChanged();
                Q_EMIT isDefaultsChanged();
                return;
            }

            const KService::Ptr service = dialog->service();
            // Check if the selected application is already in the list
            for (int i = 0; i < m_applications.length(); i++) {
                if (m_applications[i].toMap()["storageId"] == service->storageId()) {
                    m_index = i;
                    Q_EMIT indexChanged();
                    Q_EMIT isDefaultsChanged();
                    return;
                }
            }
            const QString icon = !service->icon().isEmpty() ? service->icon() : QStringLiteral("application-x-shellscript");
            QVariantMap application;
            application["name"] = service->name();
            application["icon"] = icon;
            application["storageId"] = service->storageId();
            application["execLine"] = service->exec();
            m_applications.insert(m_applications.length() - 1, application);
            m_index = m_applications.length() - 2;
            Q_EMIT applicationsChanged();
            Q_EMIT indexChanged();
            Q_EMIT isDefaultsChanged();
        });
        dialog->open();
    } else {
        m_index = index;
    }
    Q_EMIT indexChanged();
    Q_EMIT isDefaultsChanged();
}

void ComponentChooser::saveMimeTypeAssociation(const QString &mime, const QString &storageId)
{
    KSharedConfig::Ptr profile = KSharedConfig::openConfig(QStringLiteral("mimeapps.list"), KConfig::NoGlobals, QStandardPaths::GenericConfigLocation);
    if (profile->isConfigWritable(true)) {
        KConfigGroup defaultApp(profile, "Default Applications");
        defaultApp.writeXdgListEntry(mime, QStringList(storageId));

        KConfigGroup addedApps(profile, QStringLiteral("Added Associations"));
        QStringList apps = addedApps.readXdgListEntry(mime);
        apps.removeAll(storageId);
        apps.prepend(storageId); // make it the preferred app, i.e first in list
        addedApps.writeXdgListEntry(mime, apps);
        profile->sync();

        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.klauncher5"),
                                                              QStringLiteral("/KLauncher"),
                                                              QStringLiteral("org.kde.KLauncher"),
                                                              QStringLiteral("reparseConfiguration"));
        QDBusConnection::sessionBus().send(message);
    }
    m_previousApplication = m_applications[m_index].toMap()["storageId"].toString();
}

bool ComponentChooser::isDefaults() const
{
    return !m_defaultIndex.has_value() || *m_defaultIndex == m_index;
}

bool ComponentChooser::isSaveNeeded() const
{
    return !m_applications.isEmpty() && (m_previousApplication != m_applications[m_index].toMap()["storageId"].toString());
}
