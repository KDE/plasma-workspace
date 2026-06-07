/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtTest
import org.kde.plasma.workspace.timezoneselector as Workspace

Item {
    id: root
    width: 1024
    height: 768

    Workspace.TimezoneSelector {
        id: selector
        anchors.fill: parent
    }

    // -- Pre-computed band data integrity --
    TestCase {
        when: windowShown
        name: "BandData"

        // Verify that tzid data and band groups are loaded.
        function test_dataLoaded() {
            verify(Object.keys(selector.bandData.tzids).length > 0, "tzids should be non-empty")
            verify(Object.keys(selector.bandData.groups).length > 0, "groups should be non-empty")
            verify(selector.bandData.tzids["Europe/Simferopol"] !== undefined, "Europe/Simferopol should exist")
            verify(selector.bandData.tzids["Asia/Shanghai"] !== undefined, "Asia/Shanghai should exist")
            verify(selector.bandData.tzids["America/Denver"] !== undefined, "America/Denver should exist")
        }

        // Data rows for the band-coordinate validity test.
        function test_bandData_data() {
            return [
                { tag: "Europe/Simferopol", tzid: "Europe/Simferopol", maxSpan: 180 },
                { tag: "Asia/Shanghai", tzid: "Asia/Shanghai", maxSpan: 180 },
                { tag: "Asia/Anadyr", tzid: "Asia/Anadyr", maxSpan: 360 },
                { tag: "America/Denver", tzid: "America/Denver", maxSpan: 180 },
            ]
        }

        // Verify that each timezone's band coordinates are valid.
        function test_bandData(row) {
            const tz = selector.bandData.tzids[row.tzid]
            verify(tz !== undefined, row.tzid + " should have tzid data")
            verify(tz.bandMinLon !== undefined, row.tzid + " should have bandMinLon")
            verify(tz.bandMaxLon !== undefined, row.tzid + " should have bandMaxLon")
            const span = tz.bandMaxLon - tz.bandMinLon
            verify(span > 0, row.tzid + " band span should be positive")
            verify(span < row.maxSpan, row.tzid + " band span should be < " + row.maxSpan)
        }

        // Verify that the band group coordinates are valid.
        function test_groupData() {
            const tz = selector.bandData.tzids["Europe/Simferopol"]
            verify(tz !== undefined)
            const group = selector.bandData.groups[tz.bandGroup]
            verify(group !== undefined)
            verify(group.maxLon - group.minLon > 0, "Group span should be positive")
            verify(group.maxLon - group.minLon < 360, "Group span should be < 360")
        }

        // For Mountain timezones (America+-25200), verify that the per-zone
        // band coordinates match the bounding box (the special case applied
        // during data generation narrows the band to the zone's own extent).
        function test_mountainBandEqualsBbox() {
            const denver = selector.bandData.tzids["America/Denver"]
            const phoenix = selector.bandData.tzids["America/Phoenix"]
            verify(denver !== undefined)
            verify(phoenix !== undefined)
            compare(denver.bandMinLon, denver.bboxMinLon, "Denver bandMinLon should equal bboxMinLon")
            compare(denver.bandMaxLon, denver.bboxMaxLon, "Denver bandMaxLon should equal bboxMaxLon")
            compare(phoenix.bandMinLon, phoenix.bboxMinLon, "Phoenix bandMinLon should equal bboxMinLon")
            compare(phoenix.bandMaxLon, phoenix.bboxMaxLon, "Phoenix bandMaxLon should equal bboxMaxLon")
        }

        // Verify that timezones sharing the same region+offset group
        // (e.g. Asia/Seoul and Asia/Tokyo, both Asia+9) reference the
        // same bandGroup key.
        function test_bandSameGroup() {
            const seoul = selector.bandData.tzids["Asia/Seoul"]
            const tokyo = selector.bandData.tzids["Asia/Tokyo"]
            verify(seoul !== undefined)
            verify(tokyo !== undefined)
            compare(seoul.bandGroup, tokyo.bandGroup, "Seoul and Tokyo should share the same bandGroup")
        }

        // Verify that two Mountain timezones (America/Denver, America/Phoenix)
        // share the same bandGroup key.
        function test_bandMountainSameGroup() {
            const denver = selector.bandData.tzids["America/Denver"]
            const phoenix = selector.bandData.tzids["America/Phoenix"]
            verify(denver !== undefined)
            verify(phoenix !== undefined)
            compare(denver.bandGroup, phoenix.bandGroup, "Denver and Phoenix should share the same bandGroup")
            verify(denver.bandGroup === "America+-25200", "Mountain timezones should use America+-25200 group")
        }

        // Verify that timezones with the same UTC offset but different
        // region prefixes (e.g. Asia/Shanghai vs Australia/Perth, both +8)
        // reference different bandGroup keys.
        function test_bandDifferentGroup() {
            const shanghai = selector.bandData.tzids["Asia/Shanghai"]
            const perth = selector.bandData.tzids["Australia/Perth"]
            verify(shanghai !== undefined)
            verify(perth !== undefined)
            verify(shanghai.bandGroup !== perth.bandGroup,
                   "Asia/Shanghai and Australia/Perth should have different bandGroups")
        }

        // Verify that a timezone absent from the map data (e.g. a legacy
        // / deprecated alias) does not crash band lookup.
        function test_bandMissingTzid() {
            const yellowknife = selector.bandData.tzids["America/Yellowknife"]
            verify(yellowknife === undefined, "Yellowknife is not in timezone boundary data")
        }
    }

    // -- Visual rendering tests --
    TestCase {
        when: windowShown
        name: "VisualBand"

        // Pre-warm the map: wait for band data to load, then select and
        // clear a timezone so the first real test isn't penalised by
        // one-time MapPolygon setup cost.
        function initTestCase() {
            tryVerify(() => Object.keys(selector.bandData.tzids).length > 200,
                      15000, "tzids should be populated")
            selector.selectedTimeZone = "Europe/Simferopol"
            wait(0)
            selector.selectedTimeZone = ""
            wait(0)
        }

        // Verify that selecting a timezone causes a visible band to appear.
        function test_bandAppears() {
            selector.selectedTimeZone = ""
            waitForRendering(selector)
            const empty = grabImage(selector)

            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const selected = grabImage(selector)

            verify(!selected.equals(empty), "Band should change the image when selected")
        }

        // Verify that clearing the selection removes the band.
        function test_bandDisappears() {
            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const selected = grabImage(selector)

            selector.selectedTimeZone = ""
            waitForRendering(selector)
            const cleared = grabImage(selector)

            verify(!cleared.equals(selected), "Clearing selection should remove the band")
        }

        // Verify that selecting timezones from different region+offset groups
        // produces visually distinct bands on the map.
        function test_bandSwitchesBetweenTimezones() {
            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const simfImg = grabImage(selector)

            selector.selectedTimeZone = "Asia/Shanghai"
            waitForRendering(selector)
            const shangImg = grabImage(selector)

            verify(!simfImg.equals(shangImg), "Different timezones should produce different bands")
        }
    }
}