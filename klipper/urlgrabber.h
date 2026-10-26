/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QHash>
#include <QStringList>

#include <KSharedConfig>

#include "klipper_export.h"

class HistoryCycler;
class HistoryItem;
class QTimer;

class KConfig;
class QMenu;
class QMimeType;
class QRegularExpressionMatch;
class QAction;

class KLIPPER_EXPORT ClipCommand
{
public:
    /**
     * What to do with output of command
     */
    enum Output {
        IGNORE, // Discard output
        REPLACE, // Replace clipboard entry with output
        ADD, // Add output as new clipboard element
    };

    explicit ClipCommand();
    explicit ClipCommand(const QString &_command,
                         const QString &_description,
                         bool enabled = true,
                         const QString &_icon = QString(),
                         Output _output = IGNORE,
                         const QString &serviceStorageId = QString());

    QString command;
    QString description;
    bool isEnabled = true;
    QString icon;
    Output output = IGNORE;
    // If this is set, it's an app to handle a mimetype, and will be launched normally using KRun.
    // StorageId is used instead of KService::Ptr, because the latter disallows operator=.
    QString serviceStorageId;
};
Q_DECLARE_METATYPE(ClipCommand)

/**
 * Represents one configured action. An action consists of one regular
 * expression, an (optional) description and a list of ClipCommands
 * (a command to be executed, a description and an enabled/disabled flag).
 */
class KLIPPER_EXPORT ClipAction
{
public:
    explicit ClipAction(const QString &regExp = QString(), const QString &description = QString(), bool automagic = true);
    explicit ClipAction(const KSharedConfigPtr &config, const QString &group);
    ~ClipAction();

    void addCommand(const ClipCommand &cmd);

    QString actionRegexPattern;
    QStringList actionCapturedTexts;
    QString description;
    QList<ClipCommand> commands;
    bool automatic = false;
};

typedef QList<ClipAction *> ActionList;

class KLIPPER_EXPORT URLGrabber : public QObject
{
    Q_OBJECT

public:
    explicit URLGrabber(QObject *parent);
    ~URLGrabber() override;

    /**
     * Checks a given string whether it matches any of the user-defined criteria.
     * If it does, the configured action will be executed.
     */
    void checkNewData(std::shared_ptr<const HistoryItem> item);
    void invokeAction(std::shared_ptr<const HistoryItem> item);

    /**
     * Whether invokeAction() would show a menu for @p item.
     */
    bool hasMatchingActions(std::shared_ptr<const HistoryItem> item) const;

    ActionList actionList() const
    {
        return m_myActions;
    }
    void setActionList(const ActionList &);

    void loadSettings();
    void saveSettings() const;

    /**
     * Reads the configured actions from the application settings.
     */
    static ActionList loadActions();
    static void saveActions(const ActionList &actions);

    int popupTimeout() const
    {
        return m_myPopupKillTimeout;
    }
    void setPopupTimeout(int timeout)
    {
        m_myPopupKillTimeout = timeout;
    }

    bool stripWhiteSpace() const
    {
        return m_stripWhiteSpace;
    }
    void setStripWhiteSpace(bool enable)
    {
        m_stripWhiteSpace = enable;
    }

private:
    const ActionList &matchingActions(const QString &, bool automatically_invoked);
    void execute(const ClipAction *action, int commandIdx) const;
    void actionMenu(std::shared_ptr<const HistoryItem> item, bool automatically_invoked);
    void matchingMimeActions(const QString &clipData);
    /**
     * The mimetype of @p clipData, if it is a usable URL. Only absolute URLs, and
     * only existing files for file URLs, are matched.
     */
    QMimeType urlMimeType(const QString &clipData) const;
    /**
     * The item's text as the actions see it, honouring StripWhiteSpace.
     */
    QString actionText(const std::shared_ptr<const HistoryItem> &item) const;

    ActionList m_myActions;
    ActionList m_myMatches;
    std::shared_ptr<const HistoryItem> m_myClipItem;

    // holds mappings of menu action IDs to action commands (action+cmd index in it)
    QHash<QString, QPair<ClipAction *, int>> m_myCommandMapper;
    std::unique_ptr<QMenu, QScopedPointerDeleteLater> m_myMenu;
    QTimer *m_myPopupKillTimer;
    int m_myPopupKillTimeout;
    bool m_stripWhiteSpace;

private Q_SLOTS:
    void slotItemSelected(QAction *action);
    void slotKillPopupMenu();

Q_SIGNALS:
    void sigPopup(QMenu *);
};
