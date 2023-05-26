/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <KConfigWatcher>
#include <KPluginMetaData>
#include <KSharedConfig>
#include <QPointer>
#include <QQuickView>

#include <KRunner/RunnerManager>
#include <KWayland/Client/plasmashell.h>

#include <PlasmaQuick/PlasmaWindow>
#include <PlasmaQuick/SharedQmlEngine>

namespace KWayland
{
namespace Client
{
class PlasmaShell;
class PlasmaShellSurface;
}
}

class X11WindowScreenRelativePositioner;
class ViewPrivate;

class View : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.krunner.App")

    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)
    Q_PROPERTY(bool helpEnabled READ helpEnabled NOTIFY helpEnabledChanged)
    Q_PROPERTY(bool retainPriorSearch READ retainPriorSearch NOTIFY retainPriorSearchChanged)

public:
    explicit View(QWindow *parent = nullptr);
    ~View() override;

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
    void displayConfiguration();

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
    KRunner::RunnerManager *m_manager = nullptr;
    X11WindowScreenRelativePositioner *m_x11Positioner = nullptr;
};
