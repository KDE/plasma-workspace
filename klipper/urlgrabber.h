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
class QAction;

class ClipAction;
struct ClipCommand;
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

    ActionList actionList() const
    {
        return m_myActions;
    }
    void setActionList(const ActionList &);

    void loadSettings();
    void saveSettings() const;

    int popupTimeout() const
    {
        return m_myPopupKillTimeout;
    }
    void setPopupTimeout(int timeout)
    {
        m_myPopupKillTimeout = timeout;
    }

    QStringList excludedWMClasses() const
    {
        return m_myAvoidWindows;
    }
    void setExcludedWMClasses(const QStringList &list)
    {
        m_myAvoidWindows = list;
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
    bool isAvoidedWindow() const;
    void actionMenu(std::shared_ptr<const HistoryItem> item, bool automatically_invoked);
    void matchingMimeActions(const QString &clipData);

    ActionList m_myActions;
    ActionList m_myMatches;
    QStringList m_myAvoidWindows;
    std::shared_ptr<const HistoryItem> m_myClipItem;
    ClipAction *m_myCurrentAction;

    // holds mappings of menu action IDs to action commands (action+cmd index in it)
    QHash<QString, QPair<ClipAction *, int>> m_myCommandMapper;
    QMenu *m_myMenu;
    QTimer *m_myPopupKillTimer;
    int m_myPopupKillTimeout;
    bool m_stripWhiteSpace;
    HistoryCycler *m_history;

private Q_SLOTS:
    void slotItemSelected(QAction *action);
    void slotKillPopupMenu();

Q_SIGNALS:
    void sigPopup(QMenu *);
    void sigDisablePopup();
};

struct ClipCommand {
    /**
     * What to do with output of command
     */
    enum Output {
        IGNORE, // Discard output
        REPLACE, // Replace clipboard entry with output
        ADD, // Add output as new clipboard element
    };

    ClipCommand(const QString &_command,
                const QString &_description,
                bool enabled = true,
                const QString &_icon = QString(),
                Output _output = IGNORE,
                const QString &serviceStorageId = QString());

    QString command;
    QString description;
    bool isEnabled;
    QString icon;
    Output output;
    // If this is set, it's an app to handle a mimetype, and will be launched normally using KRun.
    // StorageId is used instead of KService::Ptr, because the latter disallows operator=.
    QString serviceStorageId;
};

/**
 * Represents one configured action. An action consists of one regular
 * expression, an (optional) description and a list of ClipCommands
 * (a command to be executed, a description and an enabled/disabled flag).
 */
class ClipAction
{
public:
    explicit ClipAction(const QString &regExp = QString(), const QString &description = QString(), bool automagic = true);

    ClipAction(KSharedConfigPtr kc, const QString &);
    ~ClipAction();

    QString actionRegexPattern() const
    {
        return m_regexPattern;
    }
    void setActionRegexPattern(const QString &pattern)
    {
        m_regexPattern = pattern;
    }

    QStringList actionCapturedTexts() const
    {
        return m_regexCapturedTexts;
    }
    void setActionCapturedTexts(const QStringList &captured)
    {
        m_regexCapturedTexts = captured;
    }

    void setDescription(const QString &d)
    {
        m_myDescription = d;
    }
    QString description() const
    {
        return m_myDescription;
    }

    void setAutomatic(bool automatic)
    {
        m_automatic = automatic;
    }
    bool automatic() const
    {
        return m_automatic;
    }

    /**
     * Removes all ClipCommands associated with this ClipAction.
     */
    void clearCommands()
    {
        m_myCommands.clear();
    }

    void addCommand(const ClipCommand &cmd);

    /**
     * Replaces command at index @p idx with command @p newCmd
     */
    void replaceCommand(int idx, const ClipCommand &newCmd);

    /**
     * Returns command by its index in command list
     */
    ClipCommand command(int idx) const
    {
        return m_myCommands.at(idx);
    }

    QList<ClipCommand> commands() const
    {
        return m_myCommands;
    }

    /**
     * Saves this action to a a given KConfig object
     */
    void save(KSharedConfigPtr, const QString &) const;

private:
    QString m_regexPattern;
    QStringList m_regexCapturedTexts;
    QString m_myDescription;
    QList<ClipCommand> m_myCommands;
    bool m_automatic;
};
