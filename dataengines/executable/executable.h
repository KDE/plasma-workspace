/*
    SPDX-FileCopyrightText: 2007, 2008 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KProcess>
#include <Plasma/DataContainer>
#include <Plasma/DataEngine>

class ExecutableContainer : public Plasma::DataContainer
{
    Q_OBJECT
public:
    explicit ExecutableContainer(const QString &command, QObject *parent = nullptr);
    ~ExecutableContainer() override;

protected Q_SLOTS:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void exec();

private:
    KProcess *m_process;
};

class ExecutableEngine : public Plasma::DataEngine
{
    Q_OBJECT
public:
    ExecutableEngine(QObject *parent, const QVariantList &args);

protected:
    bool sourceRequestEvent(const QString &source) override;
};
