/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ksecretprompter.h"
#include "promptcontext.h"
#include "secretprompteradaptor.h"

#include "ksecretprompter_debug.h"
#include <sys/socket.h>
#include <unistd.h>

#include <QDBusConnection>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QMessageBox>

#include <KLocalizedString>
#include <KPasswordDialog>
#include <KWindowSystem>

#include "memory.h"
#include "safe_strerror.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

void setParentWindow(QWindow *window, const QString &windowId)
{
    KWindowSystem::setMainWindow(window, windowId);
}

void setParentWindow(QWidget *widget, const QString &windowId)
{
    if (!widget->window()->windowHandle()) {
        widget->window()->winId(); // ensure we have a window handle
    }
    setParentWindow(widget->window()->windowHandle(), windowId);
}

KSecretPrompter::KSecretPrompter(QObject *parent)
    : QObject(parent)
    , m_quitTimer(new QTimer(this))
{
    m_quitTimer->setInterval(1min);
    connect(m_quitTimer, &QTimer::timeout, this, &KSecretPrompter::maybeQuit);
    m_quitTimer->start();

    new SecretprompterAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (auto ret = dbus.registerObject(u"/SecretPrompter"_s, this); !ret) {
        qCWarning(KSecretPrompterDaemon) << "Failed to register secret prompter object:" << dbus.lastError().message();
    }
}

void KSecretPrompter::Prompt(const QDBusObjectPath &path, const QString &title, const QString &prompt, const QString &windowId)
{
    auto context = make_shared_qobject<PromptContext>(message().service(), path, this);
    if (!context->isValid()) {
        qCWarning(KSecretPrompterDaemon) << "Invalid prompt context for caller:" << message().service() << "path:" << path.path();
        return;
    }
    const auto callerAddress = context->callerAddress();

    const auto id = context->id();
    connect(context.get(), &QObject::destroyed, this, [this, id]() {
        dropPrompt(id);
    });

    if (m_activePrompts.contains(id)) {
        qCWarning(KSecretPrompterDaemon) << "Prompt already existing for address:" << callerAddress << "path:" << path.path();
        return;
    }

    m_activePrompts.insert(id, context);
    qCDebug(KSecretPrompterDaemon) << "Created prompt for" << id << ", current prompts:" << m_activePrompts.size();

    qCDebug(KSecretPrompterDaemon) << "Prompt:" << path << title << prompt;

    auto dlg = make_shared_qobject<KPasswordDialog>();
    context->setWidget(dlg);
    setParentWindow(dlg.get(), windowId);
    dlg->setWindowTitle(title);
    dlg->setPrompt(prompt);
    dlg->setWindowIcon(QIcon::fromTheme(QIcon::ThemeIcon::DialogPassword));
    dlg->setModal(true);

    connect(dlg.get(), &KPasswordDialog::gotPassword, this, [this, callerAddress, id, dlg, path](const QString &password) {
        std::array<int, 2> fds{};
        if (auto ret = socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, fds.data()); ret == -1) {
            qCWarning(KSecretPrompterDaemon) << "Failed to create socketpair for prompt result:" << safe_strerror(errno);
            return;
        }
        auto closer = qScopeGuard([&] {
            for (auto fd : fds) {
                close(fd);
            }
        });

        auto &[readFd, writeFd] = fds;
        QFile writeFile;
        if (!writeFile.open(writeFd, QIODevice::WriteOnly)) {
            qCWarning(KSecretPrompterDaemon) << "Failed to open write pipe fd as QFile for prompt result:" << writeFile.errorString();
            return;
        }
        // Sockets seem to have net.core.rmem_default buffer limits, but they are super spacious. There is practically no
        // chance of exhausting them with a password.
        writeFile.write(password.toUtf8());
        if (!writeFile.flush()) {
            qCWarning(KSecretPrompterDaemon) << "Failed to write password on pipe fd result:" << writeFile.errorString();
            return;
        }

        auto message = QDBusMessage::createMethodCall(callerAddress, path.path(), u"org.kde.secretprompter.request"_s, u"Accepted"_s);
        message << QVariant::fromValue(QDBusUnixFileDescriptor(readFd));

        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [this, id](QDBusPendingCallWatcher *self) {
            self->deleteLater();
            QDBusPendingReply<int> reply = *self;
            if (reply.isError()) {
                qCWarning(KSecretPrompterDaemon) << "Failed to send prompt accepted via D-Bus:" << reply.error().message();
            }

            if (reply.value() == 0) {
                dropPrompt(id);
            } else {
                qCDebug(KSecretPrompterDaemon) << "Keeping prompt for retry as requested by client";
            }
        });
    });

    connect(dlg.get(), &QDialog::rejected, this, [this, id, callerAddress, path]() {
        auto message = QDBusMessage::createMethodCall(callerAddress, path.path(), u"org.kde.secretprompter.request"_s, u"Rejected"_s);

        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [this, id](QDBusPendingCallWatcher *self) {
            self->deleteLater();
            QDBusPendingReply<int> reply = *self;
            if (reply.isError()) {
                qCWarning(KSecretPrompterDaemon) << "Failed to send prompt rejected via D-Bus:" << reply.error().message();
            }
            if (reply.value() == 0) {
                dropPrompt(id);
            } else {
                qCDebug(KSecretPrompterDaemon) << "Keeping prompt for retry as requested by client";
            }
        });
    });

    dlg->show();
}

void KSecretPrompter::onDismissRequest()
{
    const Id promptId({message().service(), message().path()});
    dropPrompt(promptId);
}

void KSecretPrompter::UnlockCollectionPrompt(const QDBusObjectPath &path,
                                             const QString &windowId,
                                             const QString &activationToken,
                                             const QString &collectionName)
{
    Prompt(path,
           i18nc("@title:window", "Unlock Wallet"),
           xi18nc("@label", "Authentication is needed to unlock the wallet <resource>%1</resource>.", collectionName),
           windowId);
}

void KSecretPrompter::CreateCollectionPrompt(const QDBusObjectPath &path,
                                             const QString &windowId,
                                             const QString &activationToken,
                                             const QString &collectionName)
{
    Prompt(path,
           i18nc("@title", "Create Wallet"),
           xi18nc("@label", "Provide a password to create the wallet <resource>%1</resource>.", collectionName),
           windowId);
}

void KSecretPrompter::maybeQuit()
{
    if (m_activePrompts.isEmpty()) {
        qApp->quit();
    }
}

void KSecretPrompter::onRetryRequest(const QString &retryMessage)
{
    const Id promptId({message().service(), message().path()});
    m_activePrompts.value(promptId)->retry(retryMessage);
}

void KSecretPrompter::dropPrompt(const Id &id)
{
    m_activePrompts.remove(id);
    qCDebug(KSecretPrompterDaemon) << "Dropped prompt for" << id << ", remaining prompts:" << m_activePrompts.size();
    maybeQuit();
}

#include "ksecretprompter.moc"
#include "moc_ksecretprompter.cpp"
