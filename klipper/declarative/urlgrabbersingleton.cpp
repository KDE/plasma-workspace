/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "urlgrabbersingleton.h"

#include <KIO/ApplicationLauncherJob>
#include <KNotificationJobUiDelegate>
#include <KService>

#include "clipcommandprocess.h"
#include "klippersettings.h"
#include "urlgrabber.h"

using namespace Qt::StringLiterals;

URLGrabberSingleton::URLGrabberSingleton(QObject *parent)
    : QObject(parent)
{
    connect(KlipperSettings::self(), &KlipperSettings::TimeoutForActionPopupsChanged, this, &URLGrabberSingleton::popupKillTimeoutChanged);
    connect(KlipperSettings::self(), &KlipperSettings::ActionListChanged, this, &URLGrabberSingleton::loadConfig);
    loadConfig();
}

int URLGrabberSingleton::popupKillTimeout() const
{
    return KlipperSettings::timeoutForActionPopups();
}

QList<QVariantMap> URLGrabberSingleton::matchingActions(const QString &text, bool automaticallyInvoked)
{
    QList<QVariantMap> actionListMap;
    if (text.isEmpty()) {
        return actionListMap;
    }

    const ActionList actionList = URLGrabber::matchingActions(text, automaticallyInvoked, m_actions);
    for (const ClipAction &clipAction : actionList) {
        const QList<ClipCommand> &cmdList = clipAction.commands;
        for (int i = 0, listSize = cmdList.size(); i < listSize; ++i) {
            const ClipCommand &command = cmdList.at(i);
            if (!command.isEnabled) {
                continue;
            }

            QString item = command.description;
            if (item.isEmpty()) {
                item = command.command;
            }

            QVariantMap action;
            action.insert(u"actionText"_s, item);
            action.insert(u"iconText"_s, command.icon);
            action.insert(u"command"_s, QVariant::fromValue(command));
            action.insert(u"actionCapturedTexts"_s, clipAction.actionCapturedTexts);
            action.insert(u"sectionName"_s, clipAction.description);
            actionListMap.append(std::move(action));
        }
    }
    return actionListMap;
}

void URLGrabberSingleton::execute(const QString &uuid, const QString &_text, const ClipCommand &command, const QStringList &actionCapturedTexts)
{
    if (!command.isEnabled) {
        return;
    }
    QString text = _text;
    if (KlipperSettings::stripWhiteSpace()) {
        text = std::move(text).trimmed();
    }
    if (!command.serviceStorageId.isEmpty()) {
        KService::Ptr service = KService::serviceByStorageId(command.serviceStorageId);
        auto *job = new KIO::ApplicationLauncherJob(service);
        job->setUrls({QUrl(text)});
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();
    } else {
        auto *proc = new ClipCommandProcess(actionCapturedTexts, command, text, uuid);
        if (proc->program().isEmpty()) {
            delete std::exchange(proc, nullptr);
        } else {
            proc->start();
        }
    }
}

void URLGrabberSingleton::loadConfig()
{
    m_actions = URLGrabber::loadActions();
}

#include "moc_urlgrabbersingleton.cpp"
