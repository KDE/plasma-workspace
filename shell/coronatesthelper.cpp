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

#include "coronatesthelper.h"
#include "debug.h"

#include <PlasmaQuick/AppletQuickItem>

using namespace Plasma;

CoronaTestHelper::CoronaTestHelper(Corona *parent)
    : QObject(parent)
    , m_corona(parent)
    , m_exitcode(0)
{
    connect(m_corona, &Corona::startupCompleted, this, &CoronaTestHelper::initialize);
}

void CoronaTestHelper::processContainment(Plasma::Containment *containment)
{
    foreach (Plasma::Applet *applet, containment->applets()) {
        processApplet(applet);
    }
    connect(containment, &Plasma::Containment::appletAdded, this, &CoronaTestHelper::processApplet);
}

void CoronaTestHelper::processApplet(Plasma::Applet *applet)
{
    PlasmaQuick::AppletQuickItem *obj = applet->property("_plasma_graphicObject").value<PlasmaQuick::AppletQuickItem *>();
    if (applet->failedToLaunch()) {
        qCWarning(PLASMASHELL) << "cannot test an applet with a launch error" << applet->launchErrorMessage();
        qGuiApp->exit(1);
        return;
    }
    if (!obj) {
        qCWarning(PLASMASHELL) << "cannot get AppletQuickItem for applet";
        qGuiApp->exit(1);
        return;
    }

    auto testObject = obj->testItem();
    if (!testObject) {
        qCWarning(PLASMASHELL) << "no test for" << applet->title() << applet->kPackage().path();
        return;
    }
    integrateTest(testObject);
}

void CoronaTestHelper::integrateTest(QObject *testObject)
{
    if (m_registeredTests.contains(testObject))
        return;

    const int signal = testObject->metaObject()->indexOfSignal("done()");
    if (signal < 0) {
        qCWarning(PLASMASHELL) << "the test object should offer a 'done()' signal" << testObject;
        return;
    }
    if (testObject->metaObject()->indexOfProperty("failed") < 0) {
        qCWarning(PLASMASHELL) << "the test object should offer a 'bool failed' property" << testObject;
        return;
    }

    qCDebug(PLASMASHELL) << "Test registered" << testObject;
    m_tests.insert(testObject);
    m_registeredTests << testObject;

    connect(testObject, SIGNAL(done()), this, SLOT(testFinished()));
}

void CoronaTestHelper::testFinished()
{
    QObject *testObject = sender();

    const bool failed = testObject->property("failed").toBool();
    m_exitcode += failed;
    m_tests.remove(testObject);

    qCWarning(PLASMASHELL) << "test finished" << testObject << failed << "remaining" << m_tests;
    if (m_tests.isEmpty()) {
        qGuiApp->exit(m_exitcode);
    }
}

void CoronaTestHelper::initialize()
{
    foreach (Plasma::Containment *containment, m_corona->containments()) {
        processContainment(containment);
    }
    connect(m_corona, &Corona::containmentAdded, this, &CoronaTestHelper::processContainment);

    if (m_tests.isEmpty()) {
        qCWarning(PLASMASHELL) << "no tests found for the corona" << QCoreApplication::instance()->arguments();
        qGuiApp->exit();
        return;
    }
}
