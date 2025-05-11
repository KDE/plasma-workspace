#pragma once

#include <QVariantMap>

class Receiver
{
public:
    explicit Receiver(const QVariantMap &info);

    QString path() const;
    QString desktopId() const;
    uint processId() const;
    bool isSystem() const;

private:
    QString m_path;
    QString m_desktopId;
    uint m_processId;
    bool m_system;
};
