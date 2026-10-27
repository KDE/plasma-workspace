/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>

#include <QtQmlIntegration/qqmlintegration.h>

/**
 * Mirrors appium's AppiumBy / By locator strategies, so tests read like
 * find_element(By.NAME, "Icon"). It is a non-creatable helper type; the enum
 * values are accessed as `By.NAME` / `By.DESCRIPTION` / `By.ACCESSIBILITY_ID`.
 *
 * The enumerators are upper case because QML exposes them verbatim under the
 * type name: an enumerator named Name would be reached as `By.Name`, and
 * `By.NAME` would silently be undefined (which converts to Strategy::NAME, so
 * tests would pass for the wrong reason).
 */
class PlasmoidTestBy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(By)
    QML_UNCREATABLE("By is a helper type holding locator strategies; it cannot be instantiated.")

public:
    enum class Strategy {
        /** Accessible.name — appium's AppiumBy.NAME / By.name. */
        NAME,
        /** Accessible.description — appium's AppiumBy.DESCRIPTION / By.description. */
        DESCRIPTION,
        /** objectName() — appium's AppiumBy.ACCESSIBILITY_ID / By.id. */
        ACCESSIBILITY_ID,
    };
    Q_ENUM(Strategy)
};
