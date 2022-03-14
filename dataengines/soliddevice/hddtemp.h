/*
    SPDX-FileCopyrightText: 2007 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2007 Christopher Blauvelt <cblauvelt@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>

class HddTemp : public QObject
{
    Q_OBJECT

public:
    enum DataType {
        Temperature = 0,
        Unit,
    };

    explicit HddTemp(QObject *parent = nullptr);
    ~HddTemp() override;
    QStringList sources();
    QVariant data(const QString source, const DataType type) const;

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    int m_failCount = 0;
    bool m_cacheValid = false;
    QMap<QString, QList<QVariant>> m_data;
    bool updateData();
};
