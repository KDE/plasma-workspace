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
#include "historyitem.h"
#include "historymodel.h"
#include "klippersettings.h"
#include "systemclipboard.h"
#include "urlgrabber.h"

using namespace Qt::StringLiterals;

URLGrabberSingleton::URLGrabberSingleton(QObject *parent)
    : QObject(parent)
    , m_clip(SystemClipboard::self())
    , m_historyModel(HistoryModel::self())
{
    // Prevent action menu popping up on start, the top history entry is put on the
    // clipboard when Klipper starts, and that arrives as a new clipboard event.
    if (const auto top = m_historyModel->first()) {
        m_lastURLGrabberTextSelection = top->text();
        m_lastURLGrabberTextClipboard = top->text();
    }
    connect(m_historyModel.get(), &HistoryModel::changed, this, &URLGrabberSingleton::checkNewData);
    connect(KlipperSettings::self(), &KlipperSettings::URLGrabberEnabledChanged, this, [this] {
        // Forget the last text, so re-enabling the grabber offers the menu again
        // for the current clipboard contents.
        m_lastURLGrabberTextSelection.clear();
        m_lastURLGrabberTextClipboard.clear();
        Q_EMIT enabledChanged();
    });
    connect(KlipperSettings::self(), &KlipperSettings::TimeoutForActionPopupsChanged, this, &URLGrabberSingleton::popupKillTimeoutChanged);
    connect(KlipperSettings::self(), &KlipperSettings::ActionListChanged, this, &URLGrabberSingleton::loadConfig);
    loadConfig();
}

bool URLGrabberSingleton::enabled() const
{
    return KlipperSettings::uRLGrabberEnabled();
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

void URLGrabberSingleton::checkNewData(bool isTop)
{
    if (!isTop || !KlipperSettings::uRLGrabberEnabled()) {
        return;
    }

    auto item = m_historyModel->first();
    QString &lastURLGrabberText = m_clip->isLocked(QClipboard::Selection) ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if (!item || !item->allTypes().testFlag(HistoryItemType::Text)) {
        lastURLGrabberText.clear();
        return;
    }

    QString text(item->text());
    if (KlipperSettings::stripWhiteSpace()) {
        text = std::move(text).trimmed();
    }

    // Do not offer the menu for the same action twice (ie. you select and copy, or press ctrl+C twice)
    if (item->text() == lastURLGrabberText) {
        return;
    }
    lastURLGrabberText = item->text();

    const auto matchingActionsList = matchingActions(text, true);
    if (matchingActionsList.isEmpty()) {
        return;
    }

    Q_EMIT requestShowCurrentActionMenu();

    if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    if (KlipperSettings::replayActionInHistory()) {
        Q_EMIT requestShowCurrentActionMenu();
    }
}

void URLGrabberSingleton::loadConfig()
{
    m_actions = URLGrabber::loadActions();
}

#include "moc_urlgrabbersingleton.cpp"
