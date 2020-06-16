/***************************************************************************
 *   Copyright 2009 by Martin Gräßlin <kde@martin-graesslin.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#include "windowsrunner.h"

#include "config-windowsrunner.h"

#include <QTimer>

#include <QDebug>
#include <QIcon>
#include <QMutexLocker>
#include <KWindowSystem>
#include <KLocalizedString>

#if HAVE_X11
#include <QX11Info>
#include <netwm.h>
#endif

K_EXPORT_PLASMA_RUNNER_WITH_JSON(WindowsRunner, "plasma-runner-windows.json")

WindowsRunner::WindowsRunner(QObject* parent, const QVariantList& args)
    : AbstractRunner(parent, args),
      m_inSession(false)
{
    setObjectName(QStringLiteral("Windows"));

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds windows whose name, window class or window role match :q:. "
                                   "It is possible to interact with the windows by using one of the following keywords: "
                                   "activate, close, min(imize), max(imize), fullscreen, shade, keep above and keep below.")));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds windows which are on desktop named :q: "
                                   "It is possible to interact with the windows by using one of the following keywords: "
                                   "activate, close, min(imize), max(imize), fullscreen, shade, keep above and keep below.")));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Switch to desktop named :q:")));
    setDefaultSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "window"),
                                   i18n("Lists all windows and allows to activate them. "
                                   "With name=, class=, role= and desktop= the list can be reduced to "
                                   "windows matching these restrictions. "
                                   "It is possible to interact with the windows by using one of the following keywords: "
                                   "activate, close, min(imize), max(imize), fullscreen, shade, keep above and keep below.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "desktop"),
                                   i18n("Lists all other desktops and allows to switch to them.")));

    connect(this, &Plasma::AbstractRunner::prepare, this, &WindowsRunner::prepareForMatchSession);
    connect(this, &Plasma::AbstractRunner::teardown, this, &WindowsRunner::matchSessionComplete);
}

WindowsRunner::~WindowsRunner()
{
}

// Called in the main thread
void WindowsRunner::gatherInfo()
{
    if (!m_inSession) {
        return;
    }

    const auto windows = KWindowSystem::windows();
    for (const WId &w : windows) {
        KWindowInfo info(w, NET::WMWindowType | NET::WMDesktop |
                                                        NET::WMState | NET::XAWMState |
                                                        NET::WMName,
                                                    NET::WM2WindowClass | NET::WM2WindowRole | NET::WM2AllowedActions);
        if (info.valid()) {
            // ignore NET::Tool and other special window types
            NET::WindowType wType = info.windowType(NET::NormalMask | NET::DesktopMask | NET::DockMask |
                                                    NET::ToolbarMask | NET::MenuMask | NET::DialogMask |
                                                    NET::OverrideMask | NET::TopMenuMask |
                                                    NET::UtilityMask | NET::SplashMask);

            if (wType != NET::Normal && wType != NET::Override && wType != NET::Unknown &&
                wType != NET::Dialog && wType != NET::Utility) {
                continue;
            }
            m_windows.insert(w, info);
            m_icons.insert(w, QIcon(KWindowSystem::icon(w)));
        }
    }

    for (int i=1; i<=KWindowSystem::numberOfDesktops(); i++) {
        m_desktopNames << KWindowSystem::desktopName(i);
    }

    // unlock lock locked in prepareForMatchSession
    m_mutex.unlock();
}

// Called in the main thread
void WindowsRunner::prepareForMatchSession()
{
    // gatherInfo will unlock the lock
    m_mutex.lock();
    m_inSession = true;
    QTimer::singleShot(0, this, &WindowsRunner::gatherInfo);
}

// Called in the main thread
void WindowsRunner::matchSessionComplete()
{
    QMutexLocker locker(&m_mutex);
    m_inSession = false;
    m_desktopNames.clear();
    m_icons.clear();
    m_windows.clear();
}

// Called in the secondary thread
void WindowsRunner::match(Plasma::RunnerContext& context)
{
    QString term = context.query();
    if (!context.singleRunnerQueryMode() && (term.length() < 3)) {
        return;
    }

    // will run block as long as gatherInfo has not finished
    QMutexLocker locker(&m_mutex);

    QList<Plasma::QueryMatch> matches;

    // check if the search term ends with an action keyword
    WindowAction action = ActivateAction;
    if (term.endsWith(i18nc("Note this is a KRunner keyword", "activate") , Qt::CaseInsensitive)) {
        action = ActivateAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "activate")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "close") , Qt::CaseInsensitive)) {
        action = CloseAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "close")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "min") , Qt::CaseInsensitive)) {
        action = MinimizeAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "min")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "minimize") , Qt::CaseInsensitive)) {
        action = MinimizeAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "minimize")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "max") , Qt::CaseInsensitive)) {
        action = MaximizeAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "max")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "maximize") , Qt::CaseInsensitive)) {
        action = MaximizeAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "maximize")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "fullscreen") , Qt::CaseInsensitive)) {
        action = FullscreenAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "fullscreen")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "shade") , Qt::CaseInsensitive)) {
        action = ShadeAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "shade")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "keep above") , Qt::CaseInsensitive)) {
        action = KeepAboveAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "keep above")) - 1);
    } else if (term.endsWith(i18nc("Note this is a KRunner keyword", "keep below") , Qt::CaseInsensitive)) {
        action = KeepBelowAction;
        term = term.left(term.lastIndexOf(i18nc("Note this is a KRunner keyword", "keep below")) - 1);
    }

    // keyword match: when term starts with "window" we list all windows
    // the list can be restricted to windows matching a given name, class, role or desktop
    if (term.startsWith(i18nc("Note this is a KRunner keyword", "window") , Qt::CaseInsensitive)) {
        const QStringList keywords = term.split(QLatin1Char(' '));
        QString windowName;
        QString windowClass;
        QString windowRole;
        int desktop = -1;
        for (const QString& keyword : keywords) {
            if (keyword.endsWith(QLatin1Char('='))) {
                continue;
            }
            if (keyword.startsWith(i18nc("Note this is a KRunner keyword", "name") + QStringLiteral("=") , Qt::CaseInsensitive)) {
                windowName = keyword.split(QStringLiteral("="))[1];
            } else if (keyword.startsWith(i18nc("Note this is a KRunner keyword", "class") + QStringLiteral("=") , Qt::CaseInsensitive)) {
                windowClass = keyword.split(QStringLiteral("="))[1];
            } else if (keyword.startsWith(i18nc("Note this is a KRunner keyword", "role") + QStringLiteral("=") , Qt::CaseInsensitive)) {
                windowRole = keyword.split(QStringLiteral("="))[1];
            } else if (keyword.startsWith(i18nc("Note this is a KRunner keyword", "desktop") + QStringLiteral("=") , Qt::CaseInsensitive)) {
                bool ok;
                desktop = keyword.split(QStringLiteral("="))[1].toInt(&ok);
                if (!ok || desktop > KWindowSystem::numberOfDesktops()) {
                    desktop = -1; // sanity check
                }
            } else {
                // not a keyword - use as name if name is unused, but another option is set
                if (windowName.isEmpty() && !keyword.contains(QLatin1Char('=')) &&
                    (!windowRole.isEmpty() || !windowClass.isEmpty() || desktop != -1)) {
                    windowName = keyword;
                }
            }
        }
        QHashIterator<WId, KWindowInfo> it(m_windows);
        while(it.hasNext()) {
            it.next();
            WId w = it.key();
            KWindowInfo info = it.value();
            QString windowClassCompare = QString::fromUtf8(info.windowClassName()) + QLatin1Char(' ') +
                                         QString::fromUtf8(info.windowClassClass());
            // exclude not matching windows
            if (!KWindowSystem::hasWId(w)) {
                continue;
            }
            if (!windowName.isEmpty() && !info.name().startsWith(windowName, Qt::CaseInsensitive)) {
                continue;
            }
            if (!windowClass.isEmpty() && !windowClassCompare.contains(windowClass, Qt::CaseInsensitive)) {
                continue;
            }
            if (!windowRole.isEmpty() && !QString::fromUtf8(info.windowRole()).contains(windowRole, Qt::CaseInsensitive)) {
                continue;
            }
            if (desktop != -1 && !info.isOnDesktop(desktop)) {
                continue;
            }
            // check for windows when no keywords were used
            // check the name, class and role for containing the query without the keyword
            if (windowName.isEmpty() && windowClass.isEmpty() && windowRole.isEmpty() && desktop == -1) {
                const QString& test = term.mid(keywords[0].length() + 1);
                if (!info.name().contains(test, Qt::CaseInsensitive) &&
                    !windowClassCompare.contains(test, Qt::CaseInsensitive) &&
                    !QString::fromUtf8(info.windowRole()).contains(test, Qt::CaseInsensitive)) {
                    continue;
                }
            }
            // blacklisted everything else: we have a match
            if (actionSupported(info, action)){
                matches << windowMatch(info, action);
            }
        }

        if (!matches.isEmpty()) {
            // the window keyword found matches - do not process other syntax possibilities
            context.addMatches(matches);
            return;
        }
    }

    bool desktopAdded = false;
    // check for desktop keyword
    if (term.startsWith(i18nc("Note this is a KRunner keyword", "desktop") , Qt::CaseInsensitive)) {
        const QStringList parts = term.split(QLatin1Char(' '));
        if (parts.size() == 1) {
            // only keyword - list all desktops
            for (int i=1; i<=KWindowSystem::numberOfDesktops(); i++) {
                if (i == KWindowSystem::currentDesktop()) {
                    continue;
                }
                matches << desktopMatch(i);
                desktopAdded = true;
            }
        } else {
            // keyword + desktop - restrict matches
            bool isInt;
            int desktop = term.midRef(parts[0].length() + 1).toInt(&isInt);
            if (isInt && desktop != KWindowSystem::currentDesktop()) {
                matches << desktopMatch(desktop);
                desktopAdded = true;
            }
        }
    }

    // check for matches without keywords
    QHashIterator<WId, KWindowInfo> it(m_windows);
    while (it.hasNext()) {
        it.next();
        WId w = it.key();
        if (!KWindowSystem::hasWId(w)) {
            continue;
        }
        // check if window name, class or role contains the query
        KWindowInfo info = it.value();
        QString className = QString::fromUtf8(info.windowClassName());
        if (info.name().startsWith(term, Qt::CaseInsensitive) ||
            className.startsWith(term, Qt::CaseInsensitive)) {
            matches << windowMatch(info, action, 0.8, Plasma::QueryMatch::ExactMatch);
        } else if ((info.name().contains(term, Qt::CaseInsensitive) ||
             className.contains(term, Qt::CaseInsensitive)) && 
            actionSupported(info, action)) {
            matches << windowMatch(info, action, 0.7, Plasma::QueryMatch::PossibleMatch);
        }
    }

    // check for matching desktops by name
    for (const QString& desktopName : qAsConst(m_desktopNames)) {
        int desktop = m_desktopNames.indexOf(desktopName) +1;
        if (desktopName.contains(term, Qt::CaseInsensitive)) {
            // desktop name matches - offer switch to
            // only add desktops if it hasn't been added by the keyword which is quite likely
            if (!desktopAdded && desktop != KWindowSystem::currentDesktop()) {
                matches << desktopMatch(desktop, 0.8);
            }

            // search for windows on desktop and list them with less relevance
            QHashIterator<WId, KWindowInfo> it(m_windows);
            while (it.hasNext()) {
                it.next();
                KWindowInfo info = it.value();
                if (info.isOnDesktop(desktop) && actionSupported(info, action)) {
                    matches << windowMatch(info, action, 0.5, Plasma::QueryMatch::PossibleMatch);
                }
            }
        }
    }

    if (!matches.isEmpty()) {
        context.addMatches(matches);
    }
}

// Called in the main thread
void WindowsRunner::run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(context)
    // check if it's a desktop
    if (match.id().startsWith(QLatin1String("windows_desktop"))) {
        KWindowSystem::setCurrentDesktop(match.data().toInt());
        return;
    }

    const QStringList parts = match.data().toString().split(QLatin1Char('_'));
    WindowAction action = WindowAction(parts[0].toInt());
    WId w(parts[1].toULong());

    KWindowInfo info(w, NET::WMWindowType | NET::WMDesktop | NET::WMState | NET::XAWMState | NET::WMName,
                     NET::WM2WindowClass | NET::WM2WindowRole | NET::WM2AllowedActions);
    if (!info.valid()) {
        return;
    }

    switch (action) {
    case ActivateAction:
        KWindowSystem::forceActiveWindow(w);
        break;
    case CloseAction:
        {
        NETRootInfo ri(QX11Info::connection(), NET::CloseWindow);
        ri.closeWindowRequest(w);
        break;
        }
    case MinimizeAction:
        if (info.isMinimized()) {
            KWindowSystem::unminimizeWindow(w);
        } else {
            KWindowSystem::minimizeWindow(w);
        }
        break;
    case MaximizeAction:
        if (info.hasState(NET::Max)) {
            KWindowSystem::clearState(w, NET::Max);
        } else {
            KWindowSystem::setState(w, NET::Max);
        }
        break;
    case FullscreenAction:
        if (info.hasState(NET::FullScreen)) {
            KWindowSystem::clearState(w, NET::FullScreen);
        } else {
            KWindowSystem::setState(w, NET::FullScreen);
        }
        break;
    case ShadeAction:
        if (info.hasState(NET::Shaded)) {
            KWindowSystem::clearState(w, NET::Shaded);
        } else {
            KWindowSystem::setState(w, NET::Shaded);
        }
        break;
    case KeepAboveAction:
        if (info.hasState(NET::KeepAbove)) {
            KWindowSystem::clearState(w, NET::KeepAbove);
        } else {
            KWindowSystem::setState(w, NET::KeepAbove);
        }
        break;
    case KeepBelowAction:
        if (info.hasState(NET::KeepBelow)) {
            KWindowSystem::clearState(w, NET::KeepBelow);
        } else {
            KWindowSystem::setState(w, NET::KeepBelow);
        }
        break;
    }
}

Plasma::QueryMatch WindowsRunner::desktopMatch(int desktop, qreal relevance)
{
    Plasma::QueryMatch match(this);
    match.setType(Plasma::QueryMatch::ExactMatch);
    match.setData(desktop);
    match.setId(QStringLiteral("desktop-") + QString::number(desktop));
    match.setIconName(QStringLiteral("user-desktop"));
    QString desktopName;
    if (desktop <= m_desktopNames.size()) {
        desktopName = m_desktopNames[desktop - 1];
    } else {
        desktopName = KWindowSystem::desktopName(desktop);
    }
    match.setText(desktopName);
    match.setSubtext(i18n("Switch to desktop %1", desktop));
    match.setRelevance(relevance);
    return match;
}

Plasma::QueryMatch WindowsRunner::windowMatch(const KWindowInfo& info, WindowAction action, qreal relevance, Plasma::QueryMatch::Type type)
{
    Plasma::QueryMatch match(this);
    match.setType(type);
    match.setData(QString(QString::number((int)action) + QLatin1Char('_') + QString::number(info.win())));
    match.setIcon(m_icons[info.win()]);
    match.setText(info.name());
    QString desktopName;
    int desktop = info.desktop();
    if (desktop == NET::OnAllDesktops) {
        desktop = KWindowSystem::currentDesktop();
    }
    if (desktop <= m_desktopNames.size()) {
        desktopName = m_desktopNames[desktop - 1];
    } else {
        desktopName = KWindowSystem::desktopName(desktop);
    }
    switch (action) {
    case CloseAction:
        match.setSubtext(i18n("Close running window on %1", desktopName));
        break;
    case MinimizeAction:
        match.setSubtext(i18n("(Un)minimize running window on %1", desktopName));
        break;
    case MaximizeAction:
        match.setSubtext(i18n("Maximize/restore running window on %1", desktopName));
        break;
    case FullscreenAction:
        match.setSubtext(i18n("Toggle fullscreen for running window on %1", desktopName));
        break;
    case ShadeAction:
        match.setSubtext(i18n("(Un)shade running window on %1", desktopName));
        break;
    case KeepAboveAction:
        match.setSubtext(i18n("Toggle keep above for running window on %1", desktopName));
        break;
    case KeepBelowAction:
        match.setSubtext(i18n("Toggle keep below running window on %1", desktopName));
        break;
    case ActivateAction:
    default:
        match.setSubtext(i18n("Activate running window on %1", desktopName));
        break;
    }
    match.setRelevance(relevance);
    return match;
}

bool WindowsRunner::actionSupported(const KWindowInfo& info, WindowAction action)
{
    switch (action) {
    case CloseAction:
        return info.actionSupported(NET::ActionClose);
    case MinimizeAction:
        return info.actionSupported(NET::ActionMinimize);
    case MaximizeAction:
        return info.actionSupported(NET::ActionMax);
    case ShadeAction:
        return info.actionSupported(NET::ActionShade);
    case FullscreenAction:
        return info.actionSupported(NET::ActionFullScreen);
    case KeepAboveAction:
    case KeepBelowAction:
    case ActivateAction:
    default:
        return true;
    }
}

#include "windowsrunner.moc"
