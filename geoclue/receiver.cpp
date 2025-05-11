#include "receiver.h"

Receiver::Receiver(const QVariantMap &info)
    : m_path(info[QLatin1String("Client")].toString())
    , m_desktopId(info[QLatin1String("DesktopId")].toString())
    , m_processId(info[QLatin1String("ProcessId")].toUInt())
    , m_system(info[QLatin1String("System")].toBool())
{
}

QString Receiver::path() const
{
    return m_path;
}

QString Receiver::desktopId() const
{
    return m_desktopId;
}

uint Receiver::processId() const
{
    return m_processId;
}

bool Receiver::isSystem() const
{
    return m_system;
}
