/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class Klipper;

/**
 * This class registers a client for Klipper
 **/
class KlipperInterface : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit KlipperInterface(QObject *parent = nullptr);
    ~KlipperInterface() override;

    Q_INVOKABLE void configure();

private:
    std::shared_ptr<Klipper> m_klipper;
};
