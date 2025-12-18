/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDBusObjectPath>
#include <QDBusVirtualObject>
#include <QObject>

class Request : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit Request(const QDBusObjectPath &handle, QObject *parent = nullptr);
    ~Request();

    void sendCancel();

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override;
    QString introspect(const QString &path) const override;

Q_SIGNALS:
    void Cancel();

private:
    QDBusObjectPath m_handle;
};
