/*
 *  SPDX-FileCopyrightText: 2025 Oliver Beard <olib141@outlook.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

/*
 * Screen.devicePixelRatio in QML returns 2 for a screen with a scale factor of 150%, rather than the expected 1.5.
 * As a result, this helper is required to get the correct devicePixelRatio from the window (not exposed to QML).
 *
 * This can be removed when the Screen QML type returns the correct devicePixelRatio in all supported Qt versions.
 */

#pragma once

#include <QQuickWindow>

class DevicePixelRatioHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged)
    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit DevicePixelRatioHelper(QObject *parent = nullptr);

    void setWindow(QQuickWindow *window);

    QQuickWindow *window() const;
    qreal devicePixelRatio() const;

Q_SIGNALS:
    void windowChanged();
    void devicePixelRatioChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void updateDevicePixelRatio();

    QPointer<QQuickWindow> m_window = nullptr;
    qreal m_devicePixelRatio = 1.0;
};
