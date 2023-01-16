/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Plasma/Corona>

class PlasmaWindowedCorona : public Plasma::Corona
{
    Q_OBJECT

public:
    explicit PlasmaWindowedCorona(const QString &shell, QObject *parent = nullptr);
    QRect screenGeometry(int id) const override;

    void setHasStatusNotifier(bool stay);
    void loadApplet(const QString &applet, const QVariantList &arguments);

public Q_SLOTS:
    void load();
    void activateRequested(const QStringList &arguments, const QString &workingDirectory);

private:
    Plasma::Containment *m_containment = nullptr;
    bool m_hasStatusNotifier = false;
};
