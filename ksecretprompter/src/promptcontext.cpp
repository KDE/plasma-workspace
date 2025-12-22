/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "promptcontext.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QPointer>

#include "ksecretprompter_debug.h"
#include "memory.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

PromptContext::PromptContext(const QString &callerAddress, const QDBusObjectPath &path, KSecretPrompter *parent)
    : QObject(parent)
    , m_id({callerAddress, path.path()})
    , m_path(path)
    , m_watcher(new QDBusServiceWatcher(callerAddress, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this))
{
    qCDebug(KSecretPrompterDaemon) << "Creating PromptContext for caller:" << callerAddress << "path:" << path.path();

    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        m_valid = false;
        deleteLater();
    });

    QDBusConnection::sessionBus().connect(callerAddress, path.path(), u"org.kde.secretprompter.request"_s, u"Retry"_s, parent, SLOT(onRetryRequest(QString)));
    QDBusConnection::sessionBus().connect(callerAddress, path.path(), u"org.kde.secretprompter.request"_s, u"Dismiss"_s, parent, SLOT(onDismissRequest()));

    m_valid = true;
}

PromptContext::~PromptContext()
{
    qCDebug(KSecretPrompterDaemon) << "Destroying PromptContext for caller:" << m_id.first << "path:" << m_path;

    QDBusConnection::sessionBus()
        .disconnect(callerAddress(), m_path.path(), u"org.kde.secretprompter.request"_s, u"Retry"_s, parent(), SLOT(onRetryRequest(QString)));
    QDBusConnection::sessionBus()
        .disconnect(callerAddress(), m_path.path(), u"org.kde.secretprompter.request"_s, u"Dismiss"_s, parent(), SLOT(onDismissRequest()));

    auto message = QDBusMessage::createMethodCall(m_id.first, m_id.second, u"org.kde.secretprompter.request"_s, u"Dismissed"_s);
    QDBusConnection::sessionBus().asyncCall(message);
}

[[nodiscard]] bool PromptContext::isValid() const
{
    return m_valid;
}

[[nodiscard]] KSecretPrompter::Id PromptContext::id() const
{
    return m_id;
}

[[nodiscard]] QString PromptContext::callerAddress() const
{
    return m_id.first;
}

void PromptContext::setWidget(const std::shared_ptr<KPasswordDialog> &widget)
{
    m_promptWidget = widget;
}

void PromptContext::retry(const QString &message)
{
    m_promptWidget->showErrorMessage(message);
    m_promptWidget->show();
}

#include "moc_promptcontext.cpp"
