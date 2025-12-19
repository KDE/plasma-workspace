/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "promptertest.h"
#include "request.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QTimer>
#include <QWidget>

#include <KWaylandExtras>
#include <KWindowSystem>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

PrompterTest::PrompterTest(QObject *parent)
    : QObject(parent)
{
    QDBusObjectPath path(QDBusObjectPath(u"/org/kde/secretprompter/request/1"_s));
    Request *req = new Request(path);

    auto widget = new QWidget;
    widget->setMinimumSize(QSize(300, 300));
    widget->show();

    // give time to reposition the window manually to check if modality is working correctly
    QTimer::singleShot(4s, this, [this, path, widget]() {
        auto createCollectionPrompt = [path](const QString &windowId) {
            qDebug() << "Creating collection prompt with window id:" << windowId;
            QDBusInterface iface(u"org.kde.secretprompter"_s, u"/SecretPrompter"_s, u"org.kde.secretprompter"_s, QDBusConnection::sessionBus());
            iface.asyncCall(u"CreateCollectionPrompt"_s, path, windowId, /* activation token */ u""_s, /* collection name */ u"password"_s);
        };

        switch (KWindowSystem::platform()) {
        case KWindowSystem::Platform::X11: {
            const qulonglong wId = widget->winId();
            const auto windowId = QString::number(wId);
            QMetaObject::invokeMethod(this, [=] {
                createCollectionPrompt(windowId);
            });
            break;
        }
        case KWindowSystem::Platform::Wayland:
            connect(KWaylandExtras::self(), &KWaylandExtras::windowExported, this, [=](QWindow *window, const QString &handle) {
                Q_UNUSED(window)
                createCollectionPrompt(handle);
            });
            KWaylandExtras::exportWindow(widget->windowHandle());
            break;
        case KWindowSystem::Platform::Unknown:
            qWarning() << "Unknown windowing system, cannot create prompt with valid window id";
            QMetaObject::invokeMethod(this, [=] {
                createCollectionPrompt({});
            });
            break;
        }
    });
}

PrompterTest::~PrompterTest()
{
}

#include "moc_promptertest.cpp"
