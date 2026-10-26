/*
    SPDX-FileCopyrightText: 2000, 2001, 2002 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "urlgrabber.h"

#include <netwm.h>

#include "klipper_debug.h"
#include <QFile>
#include <QIcon>
#include <QMenu>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QTimer>
#include <QUuid>

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <KStringHandler>

#include "clipcommandprocess.h"
#include "klippersettings.h"

using namespace Qt::StringLiterals;

// TODO: script-interface?
#include "historycycler.h"
#include "historyitem.h"

URLGrabber::URLGrabber(QObject *parent)
    : QObject(parent)
    , m_myPopupKillTimer(new QTimer(this))
    , m_myPopupKillTimeout(8)
    , m_stripWhiteSpace(true)
{
    m_myPopupKillTimer->setSingleShot(true);
    connect(m_myPopupKillTimer, &QTimer::timeout, this, &URLGrabber::slotKillPopupMenu);
}

URLGrabber::~URLGrabber()
{
    qDeleteAll(m_myActions);
    m_myActions.clear();
}

//
// Called from Klipper::slotRepeatAction, i.e. by pressing Ctrl-Alt-R
// shortcut. I.e. never from clipboard monitoring
//
void URLGrabber::invokeAction(HistoryItemConstPtr item)
{
    actionMenu(item, false);
}

bool URLGrabber::hasMatchingActions(HistoryItemConstPtr item) const
{
    const QString text = actionText(item);

    const QMimeType mimetype = urlMimeType(text);
    if (mimetype.isValid() && !KApplicationTrader::queryByMimeType(mimetype.name()).isEmpty()) {
        return true;
    }

    for (const ClipAction *action : m_myActions) {
        if (QRegularExpression(action->actionRegexPattern).match(text).hasMatch()) {
            return true;
        }
    }
    return false;
}

QString URLGrabber::actionText(const HistoryItemConstPtr &item) const
{
    QString text(item->text());
    if (m_stripWhiteSpace) {
        text = std::move(text).trimmed();
    }
    return text;
}

void URLGrabber::setActionList(const ActionList &list)
{
    qDeleteAll(m_myActions);
    m_myActions.clear();
    m_myActions = list;
}

QMimeType URLGrabber::urlMimeType(const QString &clipData) const
{
    if (!KlipperSettings::enableMagicMimeActions()) {
        return {};
    }
    QUrl url(clipData);
    if (!url.isValid()) {
        return {};
    }
    if (url.isRelative()) { // opening a relative path will just not work. what path should be used?
        return {};
    }
    if (url.isLocalFile()) {
        if (clipData == QLatin1String("//")) {
            return {};
        }
        if (!QFile::exists(url.toLocalFile())) {
            return {};
        }
    }

    // try to figure out if clipData contains a filename
    QMimeDatabase db;
    QMimeType mimetype = db.mimeTypeForUrl(url);

    // if clipboard contents starts with http, let's assume it's "text/html".
    // That is even if we've url like "http://www.kde.org/somescript.pl", we'll
    // still treat that as html page, because determining a mimetype using kio
    // might take a long time, and i want this function to be quick!
    if ((clipData.startsWith(QLatin1String("http://")) || clipData.startsWith(QLatin1String("https://"))) && mimetype.name() != QLatin1String("text/html")) {
        mimetype = db.mimeTypeForName(QStringLiteral("text/html"));
    }

    return mimetype.isDefault() ? QMimeType() : mimetype;
}

void URLGrabber::matchingMimeActions(const QString &clipData)
{
    // let's see if we found some reasonable mimetype.
    // If we do we'll populate menu with actions for apps
    // that can handle that mimetype
    const QMimeType mimetype = urlMimeType(clipData);
    if (mimetype.isValid()) {
        const KService::List lst = KApplicationTrader::queryByMimeType(mimetype.name());
        if (!lst.isEmpty()) {
            auto *action = new ClipAction(QString(), mimetype.comment());
            for (const KService::Ptr &service : lst) {
                action->addCommand(ClipCommand(QString(), service->name(), true, service->icon(), ClipCommand::IGNORE, service->storageId()));
            }
            m_myMatches.append(action);
        }
    }
}

const ActionList &URLGrabber::matchingActions(const QString &clipData, bool automatically_invoked)
{
    m_myMatches.clear();

    // now look for matches in custom user actions
    for (ClipAction *action : m_myActions) {
        const QRegularExpressionMatch match = QRegularExpression(action->actionRegexPattern).match(clipData);
        if (match.hasMatch() && (action->automatic || !automatically_invoked)) {
            action->actionCapturedTexts = match.capturedTexts();
            m_myMatches.append(action);
        }
    }

    matchingMimeActions(clipData);

    return m_myMatches;
}

void URLGrabber::checkNewData(HistoryItemConstPtr item)
{
    actionMenu(item, true); // also creates m_myMatches
}

void URLGrabber::actionMenu(HistoryItemConstPtr item, bool automatically_invoked)
{
    if (!item) {
        qCWarning(KLIPPER_LOG, "Attempt to invoke URLGrabber without an item");
        return;
    }
    const QString text = actionText(item);
    const ActionList matchingActionsList = matchingActions(text, automatically_invoked);

    if (!matchingActionsList.isEmpty()) {
        m_myCommandMapper.clear();

        m_myPopupKillTimer->stop();

        m_myMenu.reset(new QMenu);
        m_myMenu->setWindowFlag(Qt::FramelessWindowHint, true);
        m_myMenu->setWindowFlag(Qt::Popup, false);
        m_myMenu->setObjectName(QStringLiteral("klipperActionPopup"));

        connect(m_myMenu.get(), &QMenu::triggered, this, &URLGrabber::slotItemSelected);

        for (ClipAction *clipAct : matchingActionsList) {
            m_myMenu->addSection(QIcon::fromTheme(QStringLiteral("klipper")), clipAct->description);
            const QList<ClipCommand> &cmdList = clipAct->commands;
            const int listSize = cmdList.count();
            for (int i = 0; i < listSize; ++i) {
                const ClipCommand command = cmdList.at(i);

                if (!command.isEnabled) {
                    continue;
                }

                QString item = command.description;
                if (item.isEmpty())
                    item = command.command;

                const QString id = QUuid::createUuid().toString();
                auto *action = new QAction(m_myMenu.get());
                action->setData(id);
                action->setText(item);

                if (!command.icon.isEmpty())
                    action->setIcon(QIcon::fromTheme(command.icon));

                m_myCommandMapper.insert(id, qMakePair(clipAct, i));
                m_myMenu->addAction(action);
            }
        }
        m_myMenu->addSeparator();

        auto *cancelAction = new QAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("&Cancel"), m_myMenu.get());
        m_myMenu->addAction(cancelAction);
        m_myClipItem = item;

        if (m_myPopupKillTimeout > 0)
            m_myPopupKillTimer->start(1000 * m_myPopupKillTimeout);

        Q_EMIT sigPopup(m_myMenu.get());
    }
}

void URLGrabber::slotItemSelected(QAction *action)
{
    // If it's not a popup, we need to hide it manually.
    if (!m_myMenu->windowFlags().testFlag(Qt::Popup)) {
        m_myMenu->hide();
    }

    const QString id = action->data().toString();

    if (id.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Klipper: no command associated";
        return;
    }

    // first is action ptr, second is command index
    const QPair<ClipAction *, int> actionCommand = m_myCommandMapper.value(id);

    if (actionCommand.first) {
        execute(actionCommand.first, actionCommand.second);
    } else {
        qCDebug(KLIPPER_LOG) << "Klipper: cannot find associated action";
    }
}

void URLGrabber::execute(const ClipAction *action, int cmdIdx) const
{
    if (!action) {
        qCDebug(KLIPPER_LOG) << "Action object is null";
        return;
    }

    const ClipCommand command = action->commands[cmdIdx];

    if (command.isEnabled) {
        const QString text = actionText(m_myClipItem);
        if (!command.serviceStorageId.isEmpty()) {
            KService::Ptr service = KService::serviceByStorageId(command.serviceStorageId);
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUrls({QUrl(text)});
            job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
            job->start();
        } else {
            auto *proc = new ClipCommandProcess(action->actionCapturedTexts, command, text, m_myClipItem->uuid());
            if (proc->program().isEmpty()) {
                delete std::exchange(proc, nullptr);
            } else {
                proc->start();
            }
        }
    }
}

ActionList URLGrabber::loadActions()
{
    ActionList actions;
    const KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const KConfigGroup cg(config, QStringLiteral("General"));
    for (int i = 0, num = cg.readEntry("Number of Actions", 0); i < num; i++) {
        const QString group = QStringLiteral("Action_%1").arg(i);
        actions.append(new ClipAction(config, group));
    }
    return actions;
}

void URLGrabber::saveActions(const ActionList &actions)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cg(config, u"General"_s);
    cg.writeEntry("Number of Actions", actions.size());

    for (int i = 0, count = actions.size(); i < count; ++i) {
        const ClipAction *action = actions[i];
        const QString groupName = QStringLiteral("Action_%1").arg(i);
        KConfigGroup cg(config, groupName);
        cg.writeEntry("Description", action->description);
        cg.writeEntry("Regexp", action->actionRegexPattern);
        cg.writeEntry("Number of commands", action->commands.size());
        cg.writeEntry("Automatic", action->automatic);

        // now iterate over all commands of this action
        for (int j = 0, listSize = action->commands.size(); j < listSize; ++j) {
            QString _group = groupName + QStringLiteral("/Command_%1");
            KConfigGroup cg(config, _group.arg(j));
            const ClipCommand &cmd = action->commands[j];
            cg.writePathEntry("Commandline", cmd.command);
            cg.writeEntry("Description", cmd.description);
            cg.writeEntry("Enabled", cmd.isEnabled);
            cg.writeEntry("Icon", cmd.icon);
            cg.writeEntry("Output", std::to_underlying(cmd.output));
        }
    }

    Q_EMIT KlipperSettings::self()->ActionListChanged();
}

void URLGrabber::loadSettings()
{
    m_stripWhiteSpace = KlipperSettings::stripWhiteSpace();
    m_myPopupKillTimeout = KlipperSettings::timeoutForActionPopups();

    qDeleteAll(m_myActions);
    m_myActions.clear();
    m_myActions = loadActions();
}

void URLGrabber::saveSettings() const
{
    saveActions(m_myActions);
}

void URLGrabber::slotKillPopupMenu()
{
    if (!m_myMenu) {
        return;
    }

    if (m_myPopupKillTimeout > 0 && m_myMenu->isVisible() && m_myMenu->geometry().contains(QCursor::pos())) {
        m_myPopupKillTimer->start(1000 * m_myPopupKillTimeout);
        return;
    }

    m_myMenu.reset();
}

///////////////////////////////////////////////////////////////////////////
////////

ClipCommand::ClipCommand()
{
}

ClipCommand::ClipCommand(const QString &_command,
                         const QString &_description,
                         bool _isEnabled,
                         const QString &_icon,
                         Output _output,
                         const QString &_serviceStorageId)
    : command(_command)
    , description(_description)
    , isEnabled(_isEnabled)
    , output(_output)
    , serviceStorageId(_serviceStorageId)
{
    if (!_icon.isEmpty())
        icon = _icon;
    else {
        // try to find suitable icon
        QString appName = command.section(QLatin1Char(' '), 0, 0);
        if (!appName.isEmpty()) {
            if (QIcon::hasThemeIcon(appName))
                icon = appName;
            else
                icon.clear();
        }
    }
}

ClipAction::ClipAction(const QString &regExp, const QString &description, bool automatic)
    : actionRegexPattern(regExp)
    , description(description)
    , automatic(automatic)
{
}

ClipAction::ClipAction(const KSharedConfigPtr &config, const QString &group)
    : actionRegexPattern(config->group(group).readEntry("Regexp"))
    , description(config->group(group).readEntry("Description"))
    , automatic(config->group(group).readEntry("Automatic", QVariant(true)).toBool())
{
    KConfigGroup cg(config, group);
    // read the commands
    for (int i = 0, listSize = cg.readEntry("Number of commands", 0); i < listSize; i++) {
        QString _group = group + QStringLiteral("/Command_%1");
        KConfigGroup _cg(config, _group.arg(i));

        addCommand(ClipCommand(_cg.readPathEntry("Commandline", QString()),
                               _cg.readEntry("Description"), // i18n'ed
                               _cg.readEntry("Enabled", false),
                               _cg.readEntry("Icon"),
                               static_cast<ClipCommand::Output>(_cg.readEntry("Output", QVariant(ClipCommand::IGNORE)).toInt())));
    }
}

ClipAction::~ClipAction() = default;

void ClipAction::addCommand(const ClipCommand &cmd)
{
    if (cmd.command.isEmpty() && cmd.serviceStorageId.isEmpty()) {
        return;
    }
    commands.append(cmd);
}

#include "moc_urlgrabber.cpp"
