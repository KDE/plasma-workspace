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

#include <KRunner/AbstractRunner>

#include <QMutex>

class KWindowInfo;

class WindowsRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
        WindowsRunner(QObject* parent, const QVariantList &args);
        ~WindowsRunner() override;

        void match(Plasma::RunnerContext& context) override;
        void run(const Plasma::RunnerContext& context, const Plasma::QueryMatch& match) override;

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

        QHash<WId, KWindowInfo> m_windows; // protected by m_mutex
        QHash<WId, QIcon> m_icons; // protected by m_mutex
        QStringList m_desktopNames; // protected by m_mutex
        QMutex m_mutex;

        bool m_inSession : 1; // only used in the main thread
};

#endif // WINDOWSRUNNER_H
