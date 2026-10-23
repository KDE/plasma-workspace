/*
    SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2020 Méven Car <meven.car@kdemail.net>
    SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchooserterminal.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <KQuickConfigModule>
#include <KService>
#include <kapplicationtrader.h>

#include "terminal_settings.h"

using namespace Qt::StringLiterals;

ComponentChooserTerminal::ComponentChooserTerminal(QObject *parent)
    : ComponentChooser(parent,
                       QString(),
                       QStringLiteral("TerminalEmulator"),
                       QStringLiteral("org.kde.konsole.desktop"),
                       i18n("Select default terminal emulator"))
{
}

void ComponentChooserTerminal::load()
{
    TerminalSettings terminalSettings;
    const auto preferredStorageId = terminalSettings.terminalService();

    m_model->load(m_mimeType, m_applicationCategory, m_defaultApplication, KService::KService::serviceByStorageId(preferredStorageId));

    m_index = m_model->currentIndex();

    m_currentApplication = currentStorageId();

    Q_EMIT indexChanged();
    Q_EMIT isDefaultsChanged();
}

void ComponentChooserTerminal::save()
{
    const auto modelIndex = m_model->index(m_index, 0);
    const auto storageId = m_model->data(modelIndex, ApplicationModel::StorageId).toString();
    const auto execLine = m_model->data(modelIndex, ApplicationModel::ExecLine).toString();

    TerminalSettings terminalSettings;
    terminalSettings.setTerminalApplication(execLine);
    terminalSettings.setTerminalService(storageId);
    terminalSettings.save();

    m_currentApplication = storageId;

    if (auto service = KService::KService::serviceByStorageId(storageId); service && service->supportedIntents().contains("org.freedesktop.Terminal1"_L1)) {
        KApplicationTrader::setPreferredServiceForIntent("org.freedesktop.Terminal1"_L1, service);
    } else {
        KApplicationTrader::setPreferredServiceForIntent(u"org.kde.plasma-terminalintent-wrapper"_s, service);
    }
}
