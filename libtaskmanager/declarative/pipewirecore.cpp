/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileContributor: Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pipewirecore.h"
#include "logging.h"
#include <KLocalizedString>
#include <QSocketNotifier>
#include <QThreadStorage>
#include <spa/utils/result.h>

PipeWireCore::PipeWireCore()
{
    pw_init(nullptr, nullptr);
    pwCoreEvents.version = PW_VERSION_CORE_EVENTS;
    pwCoreEvents.error = &PipeWireCore::onCoreError;
}

void PipeWireCore::onCoreError(void *data, uint32_t id, int seq, int res, const char *message)
{
    Q_UNUSED(seq)

    qCWarning(PIPEWIRE_LOGGING) << "PipeWire remote error: " << message;
    if (id == PW_ID_CORE && res == -EPIPE) {
        PipeWireCore *pw = static_cast<PipeWireCore *>(data);
        Q_EMIT pw->pipewireFailed(QString::fromUtf8(message));
    }
}

PipeWireCore::~PipeWireCore()
{
    if (pwMainLoop) {
        pw_loop_leave(pwMainLoop);
    }

    if (pwCore) {
        pw_core_disconnect(pwCore);
    }

    if (pwContext) {
        pw_context_destroy(pwContext);
    }

    if (pwMainLoop) {
        pw_loop_destroy(pwMainLoop);
    }
}

bool PipeWireCore::init()
{
    pwMainLoop = pw_loop_new(nullptr);
    pw_loop_enter(pwMainLoop);

    QSocketNotifier *notifier = new QSocketNotifier(pw_loop_get_fd(pwMainLoop), QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this] {
        int result = pw_loop_iterate(pwMainLoop, 0);
        if (result < 0)
            qCWarning(PIPEWIRE_LOGGING) << "pipewire_loop_iterate failed: " << spa_strerror(result);
    });

    pwContext = pw_context_new(pwMainLoop, nullptr, 0);
    if (!pwContext) {
        qCWarning(PIPEWIRE_LOGGING) << "Failed to create PipeWire context";
        m_error = i18n("Failed to create PipeWire context");
        return false;
    }

    pwCore = pw_context_connect(pwContext, nullptr, 0);
    if (!pwCore) {
        qCWarning(PIPEWIRE_LOGGING) << "Failed to connect PipeWire context";
        m_error = i18n("Failed to connect PipeWire context");
        return false;
    }

    if (pw_loop_iterate(pwMainLoop, 0) < 0) {
        qCWarning(PIPEWIRE_LOGGING) << "Failed to start main PipeWire loop";
        m_error = i18n("Failed to start main PipeWire loop");
        return false;
    }

    pw_core_add_listener(pwCore, &coreListener, &pwCoreEvents, this);
    return true;
}

QSharedPointer<PipeWireCore> PipeWireCore::self()
{
    static QThreadStorage<QWeakPointer<PipeWireCore>> global;
    QSharedPointer<PipeWireCore> ret;
    if (global.localData()) {
        ret = global.localData().toStrongRef();
    } else {
        ret.reset(new PipeWireCore);
        if (ret->init()) {
            global.setLocalData(ret);
        }
    }
    return ret;
}
