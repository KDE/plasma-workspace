/*
    localegenhelper.h
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once
#include <PolkitQt1/Authority>
#include <QCoreApplication>
#include <QDBusContext>
#include <QFile>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include <set>

using namespace std::chrono;
class LocaleGenHelper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.localegenhelper.LocaleGenHelper")
public:
    LocaleGenHelper();
    Q_SCRIPTABLE void enableLocales(const QStringList &locales);
Q_SIGNALS:
    Q_SCRIPTABLE void success();
    Q_SCRIPTABLE void error(const QString &msg);
private Q_SLOTS:
    void enableLocalesPrivate(PolkitQt1::Authority::Result result);

private:
    void handleLocaleGen(int statusCode, QProcess::ExitStatus status, QProcess *process);
    bool editLocaleGen();
    void exitAfterTimeOut();
    bool shouldGenerate();
    void processLocales(const QStringList &locales);

    std::atomic<bool> m_isGenerating = false;
    bool m_comment = false;
    std::set<QString> m_alreadyEnabled;
    PolkitQt1::Authority *m_authority = nullptr;
    QStringList m_locales;
    QTimer m_timer;
};
