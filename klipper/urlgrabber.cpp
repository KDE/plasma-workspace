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
#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <KStringHandler>
#include <KWindowInfo>
#include <KX11Extras>

#include "clipcommandprocess.h"
#include "klippersettings.h"

// TODO: script-interface?
#include "historycycler.h"
#include "historystringitem.h"

URLGrabber::URLGrabber(QObject *parent)
    : QObject(parent)
    , m_myCurrentAction(nullptr)
    , m_myMenu(nullptr)
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
    delete m_myMenu;
}

//
// Called from Klipper::slotRepeatAction, i.e. by pressing Ctrl-Alt-R
// shortcut. I.e. never from clipboard monitoring
//
void URLGrabber::invokeAction(HistoryItemConstPtr item)
{
    m_myClipItem = item;
    actionMenu(item, false);
}

void URLGrabber::setActionList(const ActionList &list)
{
    qDeleteAll(m_myActions);
    m_myActions.clear();
    m_myActions = list;
}

void URLGrabber::matchingMimeActions(const QString &clipData)
{
    QUrl url(clipData);
    if (!KlipperSettings::enableMagicMimeActions()) {
        return;
    }
    if (!url.isValid()) {
        return;
    }
    if (url.isRelative()) { // openinng a relative path will just not work. what path should be used?
        return;
    }
    if (url.isLocalFile()) {
        if (clipData == QLatin1String("//")) {
            return;
        }
        if (!QFile::exists(url.toLocalFile())) {
            return;
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
    if ((clipData.startsWith(QLatin1String("http://")) || clipData.startsWith(QLatin1String("https://"))) && mimetype.name() != QLatin1String("text/html")) {
        mimetype = db.mimeTypeForName(QStringLiteral("text/html"));
    }

    if (!mimetype.isDefault()) {
        const KService::List lst = KApplicationTrader::queryByMimeType(mimetype.name());
        if (!lst.isEmpty()) {
            ClipAction *action = new ClipAction(QString(), mimetype.comment());
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

    matchingMimeActions(clipData);

    // now look for matches in custom user actions
    QRegularExpression re;
    for (ClipAction *action : std::as_const(m_myActions)) {
        re.setPattern(action->actionRegexPattern());
        const QRegularExpressionMatch match = re.match(clipData);
        if (match.hasMatch() && (action->automatic() || !automatically_invoked)) {
            action->setActionCapturedTexts(match.capturedTexts());
            m_myMatches.append(action);
        }
    }

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
    QString text(item->text());
    if (m_stripWhiteSpace) {
        text = std::move(text).trimmed();
    }
    const ActionList matchingActionsList = matchingActions(text, automatically_invoked);

    if (!matchingActionsList.isEmpty()) {
        // don't react on blacklisted (e.g. konqi's/netscape's urls) unless the user explicitly asked for it
        if (automatically_invoked && isAvoidedWindow()) {
            return;
        }

        m_myCommandMapper.clear();

        m_myPopupKillTimer->stop();

        m_myMenu = new QMenu;
        m_myMenu->setWindowFlags(m_myMenu->windowFlags() | Qt::FramelessWindowHint);

        connect(m_myMenu, &QMenu::triggered, this, &URLGrabber::slotItemSelected);

        for (ClipAction *clipAct : matchingActionsList) {
            m_myMenu->addSection(QIcon::fromTheme(QStringLiteral("klipper")), clipAct->description());
            QList<ClipCommand> cmdList = clipAct->commands();
            int listSize = cmdList.count();
            for (int i = 0; i < listSize; ++i) {
                ClipCommand command = cmdList.at(i);

                QString item = command.description;
                if (item.isEmpty())
                    item = command.command;

                QString id = QUuid::createUuid().toString();
                QAction *action = new QAction(this);
                action->setData(id);
                action->setText(item);

                if (!command.icon.isEmpty())
                    action->setIcon(QIcon::fromTheme(command.icon));

                m_myCommandMapper.insert(id, qMakePair(clipAct, i));
                m_myMenu->addAction(action);
            }
        }

        // only insert this when invoked via clipboard monitoring, not from an
        // explicit Ctrl-Alt-R
        if (automatically_invoked) {
            m_myMenu->addSeparator();
            QAction *disableAction = new QAction(i18n("Disable This Popup"), this);
            connect(disableAction, &QAction::triggered, this, &URLGrabber::sigDisablePopup);
            m_myMenu->addAction(disableAction);
        }
        m_myMenu->addSeparator();

        QAction *cancelAction = new QAction(QIcon::fromTheme(QStringLiteral("dialog-cancel")), i18n("&Cancel"), this);
        connect(cancelAction, &QAction::triggered, m_myMenu, &QMenu::hide);
        m_myMenu->addAction(cancelAction);
        m_myClipItem = item;

        if (m_myPopupKillTimeout > 0)
            m_myPopupKillTimer->start(1000 * m_myPopupKillTimeout);

        Q_EMIT sigPopup(m_myMenu);
    }
}

void URLGrabber::slotItemSelected(QAction *action)
{
    if (m_myMenu)
        m_myMenu->hide(); // deleted by the timer or the next action

    QString id = action->data().toString();

    if (id.isEmpty()) {
        qCDebug(KLIPPER_LOG) << "Klipper: no command associated";
        return;
    }

    // first is action ptr, second is command index
    QPair<ClipAction *, int> actionCommand = m_myCommandMapper.value(id);

    if (actionCommand.first)
        execute(actionCommand.first, actionCommand.second);
    else
        qCDebug(KLIPPER_LOG) << "Klipper: cannot find associated action";
}

void URLGrabber::execute(const ClipAction *action, int cmdIdx) const
{
    if (!action) {
        qCDebug(KLIPPER_LOG) << "Action object is null";
        return;
    }

    ClipCommand command = action->command(cmdIdx);

    if (command.isEnabled) {
        QString text(m_myClipItem->text());
        if (m_stripWhiteSpace) {
            text = std::move(text).trimmed();
        }
        if (!command.serviceStorageId.isEmpty()) {
            KService::Ptr service = KService::serviceByStorageId(command.serviceStorageId);
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUrls({QUrl(text)});
            job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
            job->start();
        } else {
            ClipCommandProcess *proc = new ClipCommandProcess(*action, command, text, m_myClipItem);
            if (proc->program().isEmpty()) {
                delete proc;
                proc = nullptr;
            } else {
                proc->start();
            }
        }
    }
}

void URLGrabber::loadSettings()
{
    m_stripWhiteSpace = KlipperSettings::stripWhiteSpace();
    m_myAvoidWindows = KlipperSettings::noActionsForWM_CLASS();
    m_myPopupKillTimeout = KlipperSettings::timeoutForActionPopups();

    qDeleteAll(m_myActions);
    m_myActions.clear();

    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("General"));
    int num = cg.readEntry("Number of Actions", 0);
    QString group;
    for (int i = 0; i < num; i++) {
        group = QStringLiteral("Action_%1").arg(i);
        m_myActions.append(new ClipAction(KSharedConfig::openConfig(), group));
    }
}

void URLGrabber::saveSettings() const
{
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("General"));
    cg.writeEntry("Number of Actions", m_myActions.count());

    int i = 0;
    QString group;
    for (ClipAction *action : std::as_const(m_myActions)) {
        group = QStringLiteral("Action_%1").arg(i);
        action->save(KSharedConfig::openConfig(), group);
        ++i;
    }

    KlipperSettings::setNoActionsForWM_CLASS(m_myAvoidWindows);
}

// find out whether the active window's WM_CLASS is in our avoid-list
bool URLGrabber::isAvoidedWindow() const
{
    const WId active = KX11Extras::activeWindow();
    if (!active) {
        return false;
    }
    KWindowInfo info(active, NET::Properties(), NET::WM2WindowClass);
    return m_myAvoidWindows.contains(QString::fromLatin1(info.windowClassName()));
}

void URLGrabber::slotKillPopupMenu()
{
    if (m_myMenu && m_myMenu->isVisible()) {
        if (m_myMenu->geometry().contains(QCursor::pos()) && m_myPopupKillTimeout > 0) {
            m_myPopupKillTimer->start(1000 * m_myPopupKillTimeout);
            return;
        }
    }

    if (m_myMenu) {
        m_myMenu->deleteLater();
        m_myMenu = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////
////////

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
    : m_regexPattern(regExp)
    , m_myDescription(description)
    , m_automatic(automatic)
{
}

ClipAction::ClipAction(KSharedConfigPtr kc, const QString &group)
    : m_regexPattern(kc->group(group).readEntry("Regexp"))
    , m_myDescription(kc->group(group).readEntry("Description"))
    , m_automatic(kc->group(group).readEntry("Automatic", QVariant(true)).toBool())
{
    KConfigGroup cg(kc, group);

    int num = cg.readEntry("Number of commands", 0);

    // read the commands
    for (int i = 0; i < num; i++) {
        QString _group = group + QStringLiteral("/Command_%1");
        KConfigGroup _cg(kc, _group.arg(i));

        addCommand(ClipCommand(_cg.readPathEntry("Commandline", QString()),
                               _cg.readEntry("Description"), // i18n'ed
                               _cg.readEntry("Enabled", false),
                               _cg.readEntry("Icon"),
                               static_cast<ClipCommand::Output>(_cg.readEntry("Output", QVariant(ClipCommand::IGNORE)).toInt())));
    }
}

ClipAction::~ClipAction()
{
    m_myCommands.clear();
}

void ClipAction::addCommand(const ClipCommand &cmd)
{
    if (cmd.command.isEmpty() && cmd.serviceStorageId.isEmpty())
        return;

    m_myCommands.append(cmd);
}

void ClipAction::replaceCommand(int idx, const ClipCommand &cmd)
{
    if (idx < 0 || idx >= m_myCommands.count()) {
        qCDebug(KLIPPER_LOG) << "wrong command index given";
        return;
    }

    m_myCommands.replace(idx, cmd);
}

// precondition: we're in the correct action's group of the KConfig object
void ClipAction::save(KSharedConfigPtr kc, const QString &group) const
{
    KConfigGroup cg(kc, group);
    cg.writeEntry("Description", description());
    cg.writeEntry("Regexp", actionRegexPattern());
    cg.writeEntry("Number of commands", m_myCommands.count());
    cg.writeEntry("Automatic", automatic());

    int i = 0;
    // now iterate over all commands of this action
    for (const ClipCommand &cmd : std::as_const(m_myCommands)) {
        QString _group = group + QStringLiteral("/Command_%1");
        KConfigGroup cg(kc, _group.arg(i));

        cg.writePathEntry("Commandline", cmd.command);
        cg.writeEntry("Description", cmd.description);
        cg.writeEntry("Enabled", cmd.isEnabled);
        cg.writeEntry("Icon", cmd.icon);
        cg.writeEntry("Output", static_cast<int>(cmd.output));

        ++i;
    }
}
