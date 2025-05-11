#include "notifier.h"
#include "agent.h"
#include "receiver.h"

#include <KDesktopFile>
#include <KLocalizedString>

#include <QTimer>

ReceiverItem::ReceiverItem(Receiver *receiver, QObject *parent)
    : QObject(parent)
{
    const KDesktopFile desktopFile(receiver->desktopId() + QLatin1String(".desktop"));
    if (const QString name = desktopFile.readName(); !name.isEmpty()) {
        m_applicationName = name;
    } else {
        m_applicationName = QStringLiteral("%1 [PID: %2]")
            .arg(receiver->desktopId())
            .arg(receiver->processId());
    }

    QTimer::singleShot(10000, this, &ReceiverItem::expired);
}

QString ReceiverItem::applicationName() const
{
    return m_applicationName;
}

Notifier::Notifier(Agent *agent, QObject *parent)
    : KStatusNotifierItem(parent)
{
    setTitle(i18n("Geolocation Services"));
    setIconByName(QStringLiteral("mark-location-symbolic"));

    connect(agent, &Agent::receiverAdded, this, &Notifier::onReceiverAdded);
}

void Notifier::onReceiverAdded(Receiver *receiver)
{
    if (receiver->isSystem()) {
        return;
    }

    auto receiverItem = new ReceiverItem(receiver, this);
    m_receiverItems.append(receiverItem);
    connect(receiverItem, &ReceiverItem::expired, this, [this, receiverItem]() {
        m_receiverItems.removeOne(receiverItem);
        update();
    });

    update();
}

void Notifier::update()
{
    setToolTipTitle(i18n("Geolocation Services"));

    if (m_receiverItems.isEmpty()) {
        setStatus(Passive);
        setToolTipSubTitle(QString());
    } else {
        QStringList lines;
        for (const ReceiverItem *item : std::as_const(m_receiverItems)) {
            lines << i18n("%1 is using geolocation services", item->applicationName());
        }

        setToolTipSubTitle(lines.join(QLatin1Char('\n')));
        setStatus(Active);
    }
}
