/*
 *  Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CORONATESTHELPER_H
#define CORONATESTHELPER_H

#include <Plasma/Corona>
#include <QSet>

class CoronaTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit CoronaTestHelper(Plasma::Corona* parent);

    void processContainment(Plasma::Containment* containment);
    void processApplet(Plasma::Applet* applet);

private Q_SLOTS:
    void testFinished();

private:
    void initialize();
    void integrateTest(QObject* testObject);

    Plasma::Corona* m_corona;
    QSet<QObject*> m_registeredTests;
    QSet<QObject*> m_tests;

    int m_exitcode;
};

#endif
