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
#ifndef WINDOWSRUNNER_H
#define WINDOWSRUNNER_H

#include <Plasma/AbstractRunner>

class KWindowInfo;

class WindowsRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
        WindowsRunner(QObject* parent, const QVariantList &args);
        ~WindowsRunner();

        virtual void match(Plasma::RunnerContext& context);
        virtual void run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match);

    private Q_SLOTS:
        void prepareForMatchSession();
        void matchSessionComplete();
        void gatherInfo();

    private:
        enum WindowAction {
            ActivateAction,
            CloseAction,
            MinimizeAction,
            MaximizeAction,
            FullscreenAction,
            ShadeAction,
            KeepAboveAction,
            KeepBelowAction
        };
        Plasma::QueryMatch desktopMatch(int desktop, qreal relevance = 1.0);
        Plasma::QueryMatch windowMatch(const KWindowInfo& info, WindowAction action, qreal relevance = 1.0,
                                       Plasma::QueryMatch::Type type = Plasma::QueryMatch::ExactMatch);
        bool actionSupported(const KWindowInfo& info, WindowAction action);

        QHash<WId, KWindowInfo> m_windows;
        QHash<WId, QIcon> m_icons;
        QStringList m_desktopNames;

        bool m_inSession : 1;
        bool m_ready : 1;
};

K_EXPORT_PLASMA_RUNNER(windows, WindowsRunner)

#endif // WINDOWSRUNNER_H
