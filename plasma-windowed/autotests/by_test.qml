/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT

    Guards the By locator constants. QML exposes Q_ENUM enumerators verbatim
    under the type name, so a renamed enumerator leaves `By.NAME` undefined;
    findItem() would then receive 0, which is Strategy::NAME, and the tests
    using it would pass for the wrong reason.
*/

import QtQuick
import QtTest
import org.kde.plasmoidtest

Item {
    width: 10
    height: 10

    TestCase {
        when: windowShown

        function test_constants() {
            for (const name of ["NAME", "DESCRIPTION", "ACCESSIBILITY_ID"]) {
                verify(By[name] !== undefined, "By." + name + " is undefined")
            }
            verify(By.DESCRIPTION !== By.NAME, "By.DESCRIPTION collides with By.NAME")
            verify(By.ACCESSIBILITY_ID !== By.NAME, "By.ACCESSIBILITY_ID collides with By.NAME")
            verify(By.ACCESSIBILITY_ID !== By.DESCRIPTION, "By.ACCESSIBILITY_ID collides with By.DESCRIPTION")
        }
    }
}
