/*
    SPDX-FileCopyrightText: 2010 Ivan Cukic <ivan.cukic(at)kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek(at)kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QBasicTimer>
#include <QGuiApplication>
#include <QObject>

class SplashWindow;
class QQmlEngine;

class SplashApp : public QGuiApplication
{
    Q_OBJECT
public:
    explicit SplashApp(int &argc, char **argv);
    ~SplashApp() override;

protected:
    void timerEvent(QTimerEvent *event) override;
    void setStage(int stage);

private:
    int m_stage;
    QList<SplashWindow *> m_windows;
    bool m_testing;
    bool m_window;
    QBasicTimer m_timer;
    QString m_theme;
    std::shared_ptr<QQmlEngine> m_engine;

private Q_SLOTS:
    void adoptScreen(QScreen *);
};
