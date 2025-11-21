/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "urlgrabbersingleton.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>

#include <KApplicationTrader>
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
    connect(m_historyModel.get(), &HistoryModel::changed, this, &URLGrabberSingleton::checkNewData);
    connect(KlipperSettings::self(), &KlipperSettings::URLGrabberEnabledChanged, this, &URLGrabberSingleton::enabledChanged);
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

    const ActionList actionList = matchActions(text, automaticallyInvoked);
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
    if (!item || item->type() != HistoryItemType::Text) {
        lastURLGrabberText.clear();
        return;
    }

    QString text(item->text());
    if (KlipperSettings::stripWhiteSpace()) {
        text = std::move(text).trimmed();
    }

    const auto matchingActionsList = matchingActions(text, true);
    if (matchingActionsList.isEmpty()) {
        return;
    }

    Q_EMIT requestShowCurrentActionMenu();

    // Make sure URLGrabber doesn't repeat all the time if klipper reads the same
    // text all the time (e.g. because XFixes is not available and the application
    // has broken TIMESTAMP target). Using most recent history item may not always
    // work.
    if (item->text() != lastURLGrabberText) {
        lastURLGrabberText = item->text();
    }

    if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    if (KlipperSettings::replayActionInHistory()) {
        Q_EMIT requestShowCurrentActionMenu();
    }
}

ActionList URLGrabberSingleton::matchMimeActions(const QString &clipData)
{
    ActionList matches;
    QUrl url(clipData);
    if (!KlipperSettings::enableMagicMimeActions()) {
        return matches;
    }
    if (!url.isValid()) {
        return matches;
    }
    if (url.isRelative()) { // openinng a relative path will just not work. what path should be used?
        return matches;
    }
    if (url.isLocalFile()) {
        if (clipData == QLatin1String("//")) {
            return matches;
        }
        if (!QFileInfo::exists(url.toLocalFile())) {
            return matches;
        }
    }

    // try to figure out if clipData contains a filename
    QMimeDatabase db;
    QMimeType mimetype = db.mimeTypeForUrl(url);

    // let's see if we found some reasonable mimetype.
    // If we do we'll populate menu with actions for apps
    // that can handle that mimetype

    // first: if clipboard contents starts with http, let's assume it's "text/html".
    // That is even if we've url like "http://www.kde.org/somescript.pl", we'll
    // still treat that as html page, because determining a mimetype using kio
    // might take a long time, and i want this function to be quick!
    if ((clipData.startsWith(u"http://") || clipData.startsWith(u"https://")) && mimetype.name() != u"text/html") {
        mimetype = db.mimeTypeForName(u"text/html"_s);
    }

    if (!mimetype.isDefault()) {
        const KService::List lst = KApplicationTrader::queryByMimeType(mimetype.name());
        if (!lst.isEmpty()) {
            matches.reserve(lst.size());
            ClipAction action(QString(), mimetype.comment());
            for (const KService::Ptr &service : lst) {
                action.addCommand(ClipCommand(QString(), service->name(), true, service->icon(), ClipCommand::IGNORE, service->storageId()));
            }
            matches.append(std::move(action));
        }
    }

    return matches;
}

ActionList URLGrabberSingleton::matchActions(const QString &text, bool automaticallyInvoked)
{
    ActionList matches = matchMimeActions(text);

    // now look for matches in custom user actions
    QRegularExpression re;
    matches.reserve(matches.size() + m_actions.size());
    for (ClipAction action : std::as_const(m_actions)) {
        re.setPattern(action.actionRegexPattern);
        const QRegularExpressionMatch match = re.match(text);
        if (match.hasMatch() && (action.automatic || !automaticallyInvoked)) {
            action.actionCapturedTexts = match.capturedTexts();
            matches.append(std::move(action));
        }
    }

    return matches;
}

void URLGrabberSingleton::loadConfig()
{
    m_actions = URLGrabber::loadActions();
}

#include "moc_urlgrabbersingleton.cpp"
