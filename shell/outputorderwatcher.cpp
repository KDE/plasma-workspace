/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outputorderwatcher.h"
#include "debug.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>

#include <KWindowSystem>

#include "qwayland-kde-output-order-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/QtWaylandClientVersion>

#if HAVE_X11
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#endif // HAVE_X11

template<typename T>
using ScopedPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class WaylandOutputOrder : public QWaylandClientExtensionTemplate<WaylandOutputOrder>, public QtWayland::kde_output_order_v1
{
    Q_OBJECT
public:
    WaylandOutputOrder(QObject *parent)
        : QWaylandClientExtensionTemplate(1)
    {
        setParent(parent);
#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        initialize();
#else
        QMetaObject::invokeMethod(this, "addRegistryListener");
#endif
    }

protected:
    void kde_output_order_v1_output(const QString &outputName) override
    {
        if (m_done) {
            m_outputOrder.clear();
            m_done = false;
        }
        m_outputOrder.append(outputName);
    }

    void kde_output_order_v1_done() override
    {
        m_done = true;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }

Q_SIGNALS:
    void outputOrderChanged(const QStringList &outputName);

private:
    QStringList m_outputOrder;
    bool m_done = true;
};

OutputOrderWatcher::OutputOrderWatcher(QObject *parent)
    : QObject(parent)
{
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &OutputOrderWatcher::refresh);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
        // Are we in the special fake single screen situation?
        if (m_outputOrder.size() == 1 && m_outputOrder.contains(screen->name())) {
            m_outputOrder.clear();
        }
        refresh();
    });
}

void OutputOrderWatcher::useFallback(bool fallback)
{
    m_orderProtocolPresent = !fallback;
    if (fallback) {
        connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &OutputOrderWatcher::refresh, Qt::UniqueConnection);
        refresh();
    }
}

OutputOrderWatcher *OutputOrderWatcher::instance(QObject *parent)
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        return new X11OutputOrderWatcher(parent);
    } else
#endif
        if (KWindowSystem::isPlatformWayland()) {
        return new WaylandOutputOrderWatcher(parent);
    }
    // return default impl that does something at least
    return new OutputOrderWatcher(parent);
}

void OutputOrderWatcher::refresh()
{
    Q_ASSERT(!m_orderProtocolPresent);

    QStringList pendingOutputOrder;

    pendingOutputOrder.clear();
    for (auto *s : qApp->screens()) {
        pendingOutputOrder.append(s->name());
    }

    auto outputLess = [](const QString &c1, const QString &c2) {
        if (c1 == qApp->primaryScreen()->name()) {
            return true;
        } else if (c2 == qApp->primaryScreen()->name()) {
            return false;
        } else {
            return c1 < c2;
        }
    };
    std::sort(pendingOutputOrder.begin(), pendingOutputOrder.end(), outputLess);

    if (m_outputOrder != pendingOutputOrder) {
        m_outputOrder = pendingOutputOrder;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
    return;
}

QStringList OutputOrderWatcher::outputOrder() const
{
    return m_outputOrder;
}

X11OutputOrderWatcher::X11OutputOrderWatcher(QObject *parent)
    : OutputOrderWatcher(parent)
{
    // This timer is used to signal only when a qscreen for every output is already created, perhaps by monitoring
    // screenadded/screenremoved and tracking the outputs still missing
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->setInterval(0);
    connect(m_delayTimer, &QTimer::timeout, this, [this]() {
        refresh();
    });

    // By default try to use the protocol on x11
    m_orderProtocolPresent = true;

    qGuiApp->installNativeEventFilter(this);
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(QX11Info::connection(), &xcb_randr_id);
    m_xrandrExtensionOffset = reply->first_event;

    const QByteArray effectName = QByteArrayLiteral("_KDE_SCREEN_INDEX");
    xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom_unchecked(QX11Info::connection(), false, effectName.length(), effectName);
    xcb_intern_atom_reply_t *atom(xcb_intern_atom_reply(QX11Info::connection(), atomCookie, nullptr));
    if (!atom) {
        useFallback(true);
        return;
    }

    m_kdeScreenAtom = atom->atom;
    m_delayTimer->start();
}

void X11OutputOrderWatcher::refresh()
{
    if (!m_orderProtocolPresent) {
        OutputOrderWatcher::refresh();
        return;
    }
    QMap<int, QString> orderMap;

    ScopedPointer<xcb_randr_get_screen_resources_current_reply_t> reply(
        xcb_randr_get_screen_resources_current_reply(QX11Info::connection(),
                                                     xcb_randr_get_screen_resources_current(QX11Info::connection(), QX11Info::appRootWindow()),
                                                     NULL));

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply.data());
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply.data());

    for (int i = 0; i < len; i++) {
        ScopedPointer<xcb_randr_get_output_info_reply_t> output(
            xcb_randr_get_output_info_reply(QX11Info::connection(), xcb_randr_get_output_info(QX11Info::connection(), randr_outputs[i], timestamp), NULL));

        if (output == NULL || output->connection == XCB_RANDR_CONNECTION_DISCONNECTED || output->crtc == 0) {
            continue;
        }

        const auto screenName = QString::fromUtf8((const char *)xcb_randr_get_output_info_name(output.get()), xcb_randr_get_output_info_name_length(output.get()));

        auto orderCookie = xcb_randr_get_output_property(QX11Info::connection(), randr_outputs[i], m_kdeScreenAtom, XCB_ATOM_ANY, 0, 100, false, false);
        ScopedPointer<xcb_randr_get_output_property_reply_t> orderReply(xcb_randr_get_output_property_reply(QX11Info::connection(), orderCookie, nullptr));
        // If there is even a single screen without _KDE_SCREEN_INDEX info, fall back to alphabetical ordering
        if (!orderReply) {
            useFallback(true);
            return;
        }

        if (!(orderReply->type == XCB_ATOM_INTEGER && orderReply->format == 32 && orderReply->num_items == 1)) {
            useFallback(true);
            return;
        }

        const uint32_t order = *xcb_randr_get_output_property_data(orderReply.data());

        if (order > 0) { // 0 is the special case for disabled, so we ignore it
            orderMap[order] = screenName;
        }
    }

    QStringList pendingOutputOrder;

    for (const auto &screenName : orderMap) {
        pendingOutputOrder.append(screenName);
    }

    for (const auto &name : std::as_const(pendingOutputOrder)) {
        bool present = false;
        for (auto *s : qApp->screens()) {
            if (s->name() == name) {
                present = true;
                break;
            }
        }
        // if the pending output order refers to screens
        // we don't know of yet, try again next time a screen is added

        // this seems unlikely given we have the server lock and the timing thing
        if (!present) {
            m_delayTimer->start();
            return;
        }
    }

    if (pendingOutputOrder != m_outputOrder) {
        m_outputOrder = pendingOutputOrder;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
bool X11OutputOrderWatcher::nativeEventFilter(const QByteArray &eventType, void *message, long int *result)
#else
bool X11OutputOrderWatcher::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#endif
{
    Q_UNUSED(result);
    // a particular edge case: when we switch the only enabled screen
    // we don't have any signal about it, the primary screen changes but we have the same old QScreen* getting recycled
    // see https://bugs.kde.org/show_bug.cgi?id=373880
    // if this slot will be invoked many times, their//second time on will do nothing as name and primaryOutputName will be the same by then
    if (eventType[0] != 'x') {
        return false;
    }

    xcb_generic_event_t *ev = static_cast<xcb_generic_event_t *>(message);

    const auto responseType = XCB_EVENT_RESPONSE_TYPE(ev);

    if (responseType == m_xrandrExtensionOffset + XCB_RANDR_NOTIFY) {
        auto *randrEvent = reinterpret_cast<xcb_randr_notify_event_t *>(ev);
        if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
            xcb_randr_output_property_t property = randrEvent->u.op;

            if (property.atom == m_kdeScreenAtom) {
                // Force an X11 roundtrip to make sure we have all other
                // screen events in the buffer when we process the deferred refresh
                useFallback(false);
                QX11Info::getTimestamp();
                m_delayTimer->start();
            }
        }
    }
    return false;
}

WaylandOutputOrderWatcher::WaylandOutputOrderWatcher(QObject *parent)
    : OutputOrderWatcher(parent)
{
    // Asking for primaryOutputName() before this happened, will return qGuiApp->primaryScreen()->name() anyways, so set it so the outputOrderChanged will
    // have parameters that are coherent
    OutputOrderWatcher::refresh();

    auto outputListManagement = new WaylandOutputOrder(this);
    m_orderProtocolPresent = outputListManagement->isActive();
    if (!m_orderProtocolPresent) {
        useFallback(true);
        return;
    }
    connect(outputListManagement, &WaylandOutputOrder::outputOrderChanged, this, [this](const QStringList &order) {
        m_pendingOutputOrder = order;

        if (hasAllScreens()) {
            if (m_pendingOutputOrder != m_outputOrder) {
                m_outputOrder = m_pendingOutputOrder;
                Q_EMIT outputOrderChanged(m_outputOrder);
            }
        }
        // otherwse wait for next QGuiApp screenAdded/removal
        // to keep things in sync
    });
}

bool WaylandOutputOrderWatcher::hasAllScreens() const
{
    // for each name in our ordered list, find a screen with that name
    for (const auto &name : std::as_const(m_pendingOutputOrder)) {
        bool present = false;
        for (auto *s : qApp->screens()) {
            if (s->name() == name) {
                present = true;
                break;
            }
        }
        if (!present) {
            return false;
        }
    }
    return true;
}

void WaylandOutputOrderWatcher::refresh()
{
    if (!m_orderProtocolPresent) {
        OutputOrderWatcher::refresh();
        return;
    }

    if (!hasAllScreens()) {
        return;
    }

    if (m_outputOrder != m_pendingOutputOrder) {
        m_outputOrder = m_pendingOutputOrder;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
}

#include "outputorderwatcher.moc"
