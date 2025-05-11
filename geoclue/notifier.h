#pragma once

#include <KStatusNotifierItem>

class Agent;
class Receiver;

class ReceiverItem : public QObject
{
    Q_OBJECT

public:
    explicit ReceiverItem(Receiver *receiver, QObject *parent = nullptr);

    QString applicationName() const;

Q_SIGNALS:
    void expired();

private:
    QString m_applicationName;
};

class Notifier : public KStatusNotifierItem
{
    Q_OBJECT

public:
    explicit Notifier(Agent *agent, QObject *parent = nullptr);

private Q_SLOTS:
    void onReceiverAdded(Receiver *receiver);

private:
    void update();

    Agent *m_agent;
    QList<ReceiverItem *> m_receiverItems;
};
