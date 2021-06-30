/*
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#include <KWindowSystem>
#include <QHash>

/*
 * Relay class for KWindowSystem events that batches updates
 */
class XWindowSystemEventBatcher : public QObject
{
    Q_OBJECT
public:
    XWindowSystemEventBatcher(QObject *parent);
Q_SIGNALS:
    void windowAdded(WId window);
    void windowRemoved(WId window);
    void windowChanged(WId window, NET::Properties properties, NET::Properties2 properties2);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    struct AllProps {
        NET::Properties properties = {};
        NET::Properties2 properties2 = {};
    };
    QHash<WId, AllProps> m_cache;
    int m_timerId = 0;
};
