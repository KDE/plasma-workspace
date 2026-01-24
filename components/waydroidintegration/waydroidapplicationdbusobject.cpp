/*
 *   SPDX-FileCopyrightText: 2025 Florian RICHER <florian.richer@protonmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "waydroidapplicationdbusobject.h"
#include "waydroidapplicationadaptor.h"
#include "waydroidintegrationplugin_debug.h"

#include <QDBusConnection>
#include <QLoggingCategory>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

static const QRegularExpression nameRegExp(u"^Name:\\s*(\\S+)"_s);
static const QRegularExpression packageNameRegExp(u"^packageName:\\s*(\\S+)"_s);

WaydroidApplicationDBusObject::WaydroidApplicationDBusObject(QObject *parent)
    : QObject{parent}
{
}

void WaydroidApplicationDBusObject::registerObject()
{
    if (!m_dbusInitialized) {
        new WaydroidApplicationAdaptor{this};
        QString sanitizedPackageName = m_packageName;
        sanitizedPackageName.replace(u"."_s, u"_"_s);
        const QString objectPath = u"/WaydroidApplication/%1"_s.arg(sanitizedPackageName);
        QDBusConnection::sessionBus().registerObject(objectPath, this);
        m_objectPath = QDBusObjectPath(objectPath);
    }
}

void WaydroidApplicationDBusObject::unregisterObject()
{
    if (m_dbusInitialized) {
        QDBusConnection::sessionBus().unregisterObject(m_objectPath.path());
        m_dbusInitialized = false;
    }
}

QDBusObjectPath WaydroidApplicationDBusObject::objectPath() const
{
    return m_objectPath;
}

WaydroidApplicationDBusObject::Ptr WaydroidApplicationDBusObject::parseApplicationFromWaydroidLog(QTextStream &inFile)
{
    const QString line = inFile.readLine();
    const QRegularExpressionMatch nameMatch = nameRegExp.match(line);

    if (!nameMatch.hasMatch() || nameMatch.lastCapturedIndex() == 0) {
        return nullptr;
    }

    auto app = std::make_shared<WaydroidApplicationDBusObject>();
    app->m_name = nameMatch.captured(nameMatch.lastCapturedIndex());

    qint64 oldPos = inFile.pos();
    while (!inFile.atEnd()) {
        const QString line = inFile.readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QRegularExpressionMatch nameMatch = nameRegExp.match(line);
        if (nameMatch.hasMatch()) {
            inFile.seek(oldPos); // Revert file cursor position for the next Application parsing
            return app;
        }

        const QRegularExpressionMatch packageNameMatch = packageNameRegExp.match(line);
        if (packageNameMatch.hasMatch() && packageNameMatch.lastCapturedIndex() > 0) {
            app->m_packageName = packageNameMatch.captured(packageNameMatch.lastCapturedIndex());
        }

        oldPos = inFile.pos();
    }

    return app;
}

QList<WaydroidApplicationDBusObject::Ptr> WaydroidApplicationDBusObject::parseApplicationsFromWaydroidLog(QTextStream &inFile)
{
    QList<Ptr> applications;
    while (!inFile.atEnd()) {
        const auto app = parseApplicationFromWaydroidLog(inFile);
        if (app == nullptr) {
            qCWarning(WAYDROIDINTEGRATIONPLUGIN) << "Failed to fetch the application: Maybe wrong QTextStream cursor position.";
            break;
        }

        qCDebug(WAYDROIDINTEGRATIONPLUGIN) << "Waydroid application found: " << app.get()->name() << " (" << app.get()->packageName() << ")";
        applications.append(app);
    }
    return applications;
}

QString WaydroidApplicationDBusObject::name() const
{
    return m_name;
}

QString WaydroidApplicationDBusObject::packageName() const
{
    return m_packageName;
}
