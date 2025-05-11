#include "agent.h"
#include "logging.h"
#include "manager.h"
#include "receiver.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDebug>
#include <QGuiApplication>

Agent::Agent(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::systemBus())
    , m_serviceWatcher(new QDBusServiceWatcher(QStringLiteral("org.freedesktop.GeoClue2"), m_bus))
{
    qDBusRegisterMetaType<QList<QVariantMap>>();

    connect(m_serviceWatcher.get(), &QDBusServiceWatcher::serviceRegistered, this, &Agent::onManagerRegistered);
    connect(m_serviceWatcher.get(), &QDBusServiceWatcher::serviceUnregistered, this, &Agent::onManagerUnregistered);

    m_bus.registerObject(QStringLiteral("/org/freedesktop/GeoClue2/Agent"),
                         QStringLiteral("org.freedesktop.GeoClue2.Agent"),
                         this,
                         QDBusConnection::ExportScriptableProperties | QDBusConnection::ExportScriptableSlots);

    if (m_bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.GeoClue2"))) {
        onManagerRegistered();
    }
}

Agent::~Agent()
{
    qDeleteAll(m_receivers);
}

uint Agent::MaxAccuracyLevel() const
{
    return 1;
}

bool Agent::AuthorizeApp(const QString &desktopId, uint requestedAccuracyLevel, uint &allowedAccuracyLevel)
{
    qDebug() << "authorize" << desktopId;
    allowedAccuracyLevel = requestedAccuracyLevel;
    return true;
}

void Agent::onManagerRegistered()
{
    qDebug() << "manager available";

    m_manager = std::make_unique<OrgFreedesktopGeoClue2ManagerInterface>(QStringLiteral("org.freedesktop.GeoClue2"), QStringLiteral("/org/freedesktop/GeoClue2/Manager"), m_bus);

    auto watcher = new QDBusPendingCallWatcher(m_manager->AddAgent(QGuiApplication::desktopFileName()), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();

        if (self->isError()) {
            qCWarning(AGENT) << "Failed to register as an agent:" << self->error();
        } else {
            onAgentReady();
        }
    });
}

void Agent::onManagerUnregistered()
{
    qDebug() << "manager unavailable";

    m_manager.reset();

    qDeleteAll(m_receivers);
    m_receivers.clear();
}

void Agent::onAgentReady()
{
    qDebug() << "registered as an agent";

    connect(m_manager.get(), &OrgFreedesktopGeoClue2ManagerInterface::ReceiverNew, this, &Agent::onReceiverNew);
    connect(m_manager.get(), &OrgFreedesktopGeoClue2ManagerInterface::ReceiverRemoved, this, &Agent::onReceiverRemoved);

    auto query = new QDBusPendingCallWatcher(m_manager->ListReceivers(), this);
    connect(query, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();

        QDBusPendingReply<QList<QVariantMap>> reply = *self;
        if (reply.isError()) {
            qCWarning(AGENT) << "Failed to query list of receiving clients:" << reply.error();
        } else {
            for (const QVariantMap &info : reply.value()) {
                m_receivers.emplace_back(new Receiver(info));
            }
        }
    });
}

void Agent::onReceiverNew(const QVariantMap &info)
{
    qDebug() << "receiver new:" << info;

    auto receiver = new Receiver(info);
    m_receivers.emplace_back(receiver);
    Q_EMIT receiverAdded(receiver);
}

void Agent::onReceiverRemoved(const QVariantMap &info)
{
    qDebug() << "receiver removed:" << info;

    const QString path = info[QLatin1String("Client")].toString();

    auto it = std::find_if(m_receivers.begin(), m_receivers.end(), [path](const Receiver *receiver) {
        return receiver->path() == path;
    });
    if (it != m_receivers.end()) {
        Receiver *receiver = *it;
        m_receivers.erase(it);
        Q_EMIT receiverRemoved(receiver);
        delete receiver;
    }
}

#include "moc_agent.cpp"
