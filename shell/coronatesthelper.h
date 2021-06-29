/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Corona>
#include <QSet>

class CoronaTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit CoronaTestHelper(Plasma::Corona *parent);

    void processContainment(Plasma::Containment *containment);
    void processApplet(Plasma::Applet *applet);

private Q_SLOTS:
    void testFinished();

private:
    void initialize();
    void integrateTest(QObject *testObject);

    Plasma::Corona *m_corona;
    QSet<QObject *> m_registeredTests;
    QSet<QObject *> m_tests;

    int m_exitcode;
};
