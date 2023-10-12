/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <KConfigWatcher>
#include <KPluginMetaData>
#include <KSharedConfig>
#include <QQuickView>

#include <PlasmaQuick/PlasmaWindow>
#include <PlasmaQuick/SharedQmlEngine>

class X11WindowScreenRelativePositioner;
class ViewPrivate;

class View : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.krunner.App")

    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)
    Q_PROPERTY(bool helpEnabled READ helpEnabled NOTIFY helpEnabledChanged)
    Q_PROPERTY(bool retainPriorSearch READ retainPriorSearch NOTIFY retainPriorSearchChanged)
    Q_PROPERTY(HistoryBehavior historyBehavior READ historyBehavior NOTIFY historyBehaviorChanged)
    Q_PROPERTY(QStringList favoriteIds MEMBER m_favoriteIds NOTIFY favoriteIdsChanged)

public:
    explicit View(QWindow *parent = nullptr);
    ~View() override;

    enum HistoryBehavior {
        Disabled,
        ImmediateCompletion,
        CompletionSuggestion,
    };
    Q_ENUM(HistoryBehavior)

    void positionOnScreen();

    bool freeFloating() const;
    void setFreeFloating(bool floating);

    bool pinned() const;
    void setPinned(bool pinned);

    bool helpEnabled()
    {
        const static auto metaData = KPluginMetaData(QStringLiteral("kf6/krunner/helprunner"));
        const KConfigGroup grp = KSharedConfig::openConfig()->group("Plugins");
        return metaData.isEnabled(grp);
    }
    HistoryBehavior historyBehavior()
    {
        return m_historyBehavior;
    }
    void setHistoryBehavior(HistoryBehavior behavior)
    {
        m_historyBehavior = behavior;
        Q_EMIT historyBehaviorChanged();
    }
    Q_SIGNAL void historyBehaviorChanged();

    Q_SIGNAL void retainPriorSearchChanged();
    bool retainPriorSearch() const
    {
        return m_retainPriorSearch;
    }
    void setRetainPriorSearch(bool retain)
    {
        m_retainPriorSearch = retain;
        Q_EMIT retainPriorSearchChanged();
    }

    Q_SIGNAL void favoriteIdsChanged();
    void assignFavoriteIds()
    {
        const KConfigGroup grp = m_config.parent().group("Plugins").group("Favorites");
        m_favoriteIds = grp.readEntry("plugins", QStringList(QStringLiteral("krunner_services")));
        Q_EMIT favoriteIdsChanged();
    }

Q_SIGNALS:
    void pinnedChanged();
    void helpEnabledChanged();

protected:
    void showEvent(QShowEvent *event) override;

public Q_SLOTS:
    void display();
    void toggleDisplay();
    void displaySingleRunner(const QString &runnerName);
    void displayWithClipboardContents();
    void query(const QString &term);
    void querySingleRunner(const QString &runnerName, const QString &term);

protected Q_SLOTS:
    void loadConfig();
    void objectIncubated();
    void slotFocusWindowChanged();

private:
    void writeHistory();
    QPoint m_customPos;
    PlasmaQuick::SharedQmlEngine *m_engine;
    KConfigGroup m_config;
    KConfigGroup m_stateData;
    KConfigWatcher::Ptr m_configWatcher;
    bool m_floating = false;
    bool m_pinned = false;
    bool m_retainPriorSearch = false;
    bool m_requestedClipboardSelection = false;
    QStringList m_history;
    QStringList m_favoriteIds;
    X11WindowScreenRelativePositioner *m_x11Positioner = nullptr;
    HistoryBehavior m_historyBehavior = HistoryBehavior::CompletionSuggestion;
};
