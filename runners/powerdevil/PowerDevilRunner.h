/***************************************************************************
 *   Copyright (C) 2008 by Dario Freddi <drf@kdemod.ath.cx>                *
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

#ifndef POWERDEVILRUNNER_H
#define POWERDEVILRUNNER_H

#include <KRunner/AbstractRunner>
#include <QDBusConnection>

typedef QMap< QString, QString > StringStringMap;

class PowerDevilRunner : public Plasma::AbstractRunner
{
        Q_OBJECT

    public:
        PowerDevilRunner( QObject *parent, const QVariantList &args );
        ~PowerDevilRunner() override;

        void match( Plasma::RunnerContext &context ) override;
        void run( const Plasma::RunnerContext &context, const Plasma::QueryMatch &action ) override;

    private Q_SLOTS:
        void updateStatus();

    private:
        void initUpdateTriggers();
        void updateSyntaxes();
        void addSuspendMatch(int value, QList<Plasma::QueryMatch> &matches);
        bool parseQuery(const QString& query, const QList<QRegExp>& rxList, QString& parameter) const;

        int m_shortestCommand;
};

Q_DECLARE_METATYPE(StringStringMap)

#endif
