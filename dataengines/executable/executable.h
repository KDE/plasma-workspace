/*
 *   Copyright (C) 2007, 2008 Petri Damsten <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef EXECUTABLE_DATAENGINE_H
#define EXECUTABLE_DATAENGINE_H

#include <KProcess>
#include <Plasma/DataEngine>
#include <Plasma/DataContainer>

class ExecutableContainer : public Plasma::DataContainer
{
    Q_OBJECT
    public:
        explicit ExecutableContainer(const QString& command, QObject *parent = nullptr);
        ~ExecutableContainer() override;

    protected Q_SLOTS:
        void finished(int exitCode, QProcess::ExitStatus exitStatus);
        void exec();

    private:
        KProcess* m_process;
};

class ExecutableEngine : public Plasma::DataEngine
{
    Q_OBJECT
    public:
        ExecutableEngine(QObject *parent, const QVariantList &args);

    protected:
        bool sourceRequestEvent(const QString& source) override;
};

#endif
