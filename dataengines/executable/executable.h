/*
    SPDX-FileCopyrightText: 2007, 2008 Petri Damsten <damu@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QPointer>

#include <KProcess>
#include <Plasma5Support/DataContainer>
#include <Plasma5Support/DataEngine>

class KNotification;

class ExecutableContainer : public Plasma5Support::DataContainer
{
    Q_OBJECT
public:
    explicit ExecutableContainer(const QString &command, QObject *parent = nullptr);
    ~ExecutableContainer() override;

private:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void aboutToExec();
    void exec();
    void deny();

    KProcess *m_process = nullptr;
    QPointer<KNotification> m_notification;
};

class ExecutableEngine : public Plasma5Support::DataEngine
{
    Q_OBJECT
public:
    ExecutableEngine(QObject *parent);

protected:
    bool sourceRequestEvent(const QString &source) override;
};
