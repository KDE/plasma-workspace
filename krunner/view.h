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

#include <PlasmaQuick/Dialog>

namespace KDeclarative
{
class QmlObject;
}

namespace KWayland
{
namespace Client
{
class PlasmaShell;
class PlasmaShellSurface;
}
}

class ViewPrivate;

class View : public PlasmaQuick::Dialog
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.krunner.App")

    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    // TODO KF6 This is kept for compatibility with third party themes which override the RunCommand.qml file
    Q_PROPERTY(Plasma::RunnerManager *runnerManager WRITE setRunnerManager)
    Q_PROPERTY(bool helpEnabled READ helpEnabled NOTIFY helpEnabledChanged)
    Q_PROPERTY(int runnerWidth READ runnerWidth NOTIFY runnerWidthChanged)


public:
    explicit View(QWindow *parent = nullptr);
    ~View() override;

    void positionOnScreen();

    bool freeFloating() const;
    void setFreeFloating(bool floating);

    QStringList history() const;

    bool pinned() const;
    void setPinned(bool pinned);

    // TODO KF6 This is kept for compatibility with third party themes which override the RunCommand.qml file
    Q_SIGNAL void historyChanged();
    Q_INVOKABLE void addToHistory(const QString &)
    {
        // Kept for compatibility, since milou f442b33af3c292c49743083493423275a51c118a the KRunner framework logic is used for handling this
    }
    Q_INVOKABLE void removeFromHistory(int index);
    void setRunnerManager(Plasma::RunnerManager *manager)
    {
        m_manager = manager;
    }

    bool helpEnabled()
    {
        const static auto metaData = KPluginMetaData(QStringLiteral("kf5/krunner/helprunner"));
        const KConfigGroup grp = KSharedConfig::openConfig()->group("Plugins");
        return metaData.isEnabled(grp);
    }

    int runnerWidth() const;


Q_SIGNALS:
    void pinnedChanged();
    void helpEnabledChanged();
    void runnerWidthChanged();

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

public Q_SLOTS:
    void setVisible(bool visible);
    void display();
    void toggleDisplay();
    void displaySingleRunner(const QString &runnerName);
    void displayWithClipboardContents();
    void query(const QString &term);
    void querySingleRunner(const QString &runnerName, const QString &term);
    void switchUser();
    void displayConfiguration();

protected Q_SLOTS:
    void screenGeometryChanged();
    void resetScreenPos();
    void loadConfig();
    void objectIncubated();
    void slotFocusWindowChanged();

private:
    void writeHistory();
    QPoint m_customPos;
    KDeclarative::QmlObject *m_qmlObj;
    KConfigGroup m_config;
    KConfigGroup m_stateData;
    KConfigWatcher::Ptr m_configWatcher;
    qreal m_offset;
    bool m_floating : 1;
    bool m_requestedVisible = false;
    bool m_pinned = false;
    QStringList m_history;
    Plasma::RunnerManager *m_manager = nullptr;
    int m_runnerWidth;
};
