#pragma once

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QObject>

class OrgFreedesktopGeoClue2ManagerInterface;
class Receiver;

class Agent : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.GeoClue2.Agent")
    Q_PROPERTY(uint MaxAccuracyLevel READ MaxAccuracyLevel)

public:
    explicit Agent(QObject *parent = nullptr);
    ~Agent() override;

    uint MaxAccuracyLevel() const;

Q_SIGNALS:
    void receiverAdded(Receiver *receiver);
    void receiverRemoved(Receiver *receiver);

public Q_SLOTS:
    bool AuthorizeApp(const QString &desktopId, uint requestedAccuracyLevel, uint &allowedAccuracyLevel);

private Q_SLOTS:
    void onManagerRegistered();
    void onManagerUnregistered();
    void onAgentReady();
    void onReceiverNew(const QVariantMap &info);
    void onReceiverRemoved(const QVariantMap &info);

private:
    QDBusConnection m_bus;
    std::unique_ptr<QDBusServiceWatcher> m_serviceWatcher;
    std::unique_ptr<OrgFreedesktopGeoClue2ManagerInterface> m_manager;
    std::vector<Receiver *> m_receivers;
};
