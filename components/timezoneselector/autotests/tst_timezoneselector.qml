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

    // -- Pre-computed group data integrity --
    TestCase {
        when: windowShown
        name: "GroupData"

        // Verify that tzid metadata and group outline data are loaded.
        function test_dataLoaded() {
            verify(Object.keys(selector.bandData.tzids).length > 0, "tzid data should be non-empty")
            verify(selector.bandData.tzids["Europe/Simferopol"] !== undefined, "Europe/Simferopol should exist")
            verify(selector.bandData.tzids["Asia/Shanghai"] !== undefined, "Asia/Shanghai should exist")
        }

        // Data rows for the bounding-box validity test.
        function test_bboxData_data() {
            return [
                { tag: "Europe/Simferopol", tzid: "Europe/Simferopol", maxSpan: 180, normalizedMinLon: false },
                { tag: "Asia/Shanghai", tzid: "Asia/Shanghai", maxSpan: 180, normalizedMinLon: false },
                { tag: "Asia/Anadyr", tzid: "Asia/Anadyr", maxSpan: 360, normalizedMinLon: true },
            ]
        }

        // Verify that each timezone's pre-computed bounding box has a
        // positive longitude span within the expected upper bound.
        function test_bboxData(row) {
            const tz = selector.bandData.tzids[row.tzid]
            verify(tz !== undefined, row.tzid + " should have tzid data")
            const span = tz.bboxMaxLon - tz.bboxMinLon
            verify(span > 0, row.tzid + " bbox span should be positive")
            verify(span < row.maxSpan, row.tzid + " bbox span should be < " + row.maxSpan)
            if (row.normalizedMinLon) {
                verify(tz.bboxMinLon >= 0, row.tzid + " normalized bboxMinLon should be >= 0")
            }
        }

        // Verify that centerLat/centerLon are populated and numeric.
        function test_centerCoordinates() {
            const sh = selector.bandData.tzids["Asia/Shanghai"]
            verify(sh !== undefined)
            verify(typeof sh.centerLat === "number", "centerLat should be a number")
            verify(typeof sh.centerLon === "number", "centerLon should be a number")
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

        // Verify that pre-computed group outlines are present and non-empty.
        function test_outlinesExist() {
            const out = selector.bandData.outlines
            verify(out !== undefined, "group outlines should exist")
            verify(Object.keys(out).length > 0, "group outlines should be non-empty")
        }

        // Verify that selectedOutlinePaths is empty when no timezone is selected.
        function test_selectedOutlinePathsNoSelection() {
            selector.selectedTimeZone = ""
            verify(selector.selectedOutlinePaths.length === 0, "selectedOutlinePaths should be empty with no selection")
        }

        // Verify that selectedOutlinePaths returns the outline of the
        // selected timezone's band group.
        function test_selectedOutlinePathsWithSelection() {
            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const paths = selector.selectedOutlinePaths
            verify(paths.length > 0, "selectedOutlinePaths should be non-empty for selected timezone")
        }
    }

    // -- Visual highlighting tests --
    TestCase {
        when: windowShown
        name: "VisualHighlight"

        // Pre-warm the map: wait for tzid data to load, then select and
        // clear a timezone so the first real test isn't penalised by
        // one-time rendering setup cost.
        function initTestCase() {
            tryVerify(() => Object.keys(selector.bandData.tzids).length > 200,
                       15000, "tzid data should be populated")
            selector.selectedTimeZone = "Europe/Simferopol"
            wait(0)
            selector.selectedTimeZone = ""
            wait(0)
        }

        // Verify that selecting a timezone causes visible highlighting.
        function test_highlightAppears() {
            selector.selectedTimeZone = ""
            waitForRendering(selector)
            const empty = grabImage(selector)

            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const selected = grabImage(selector)

            verify(!selected.equals(empty), "Highlighting should change the image when selected")
        }

        // Verify that clearing the selection removes the highlight.
        function test_highlightDisappears() {
            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const selected = grabImage(selector)

            selector.selectedTimeZone = ""
            waitForRendering(selector)
            const cleared = grabImage(selector)

            verify(!cleared.equals(selected), "Clearing selection should remove the highlighting")
        }

        // Verify that selecting timezones from different region+offset groups
        // produces visually distinct highlights on the map.
        function test_highlightSwitchesBetweenTimezones() {
            selector.selectedTimeZone = "Europe/Simferopol"
            waitForRendering(selector)
            const simfImg = grabImage(selector)

            selector.selectedTimeZone = "Asia/Shanghai"
            waitForRendering(selector)
            const shangImg = grabImage(selector)

            verify(!simfImg.equals(shangImg), "Different timezones should produce different highlights")
        }

        // Verify that selecting timezones within the same group still
        // updates the visual highlight on the map.
        function test_sameGroupHighlight() {
            selector.selectedTimeZone = "Asia/Seoul"
            waitForRendering(selector)
            const seoulImg = grabImage(selector)

            selector.selectedTimeZone = "Asia/Tokyo"
            waitForRendering(selector)
            const tokyoImg = grabImage(selector)

            verify(!seoulImg.equals(tokyoImg), "Same group selection should still update highlights")
        }
    }
}
