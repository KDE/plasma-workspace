# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

include(ECMAddTests)

ecm_add_test(chrome/testchromebookmarks.cpp TEST_NAME testChromeBookmarks
    LINK_LIBRARIES Qt::Test krunner_bookmarks_common
)

ecm_add_test(firefox/testfirefoxbookmarks.cpp TEST_NAME testFirefoxBookmarks
    LINK_LIBRARIES Qt::Test krunner_bookmarks_common
)

ecm_add_test(bookmarksmatchtest.cpp TEST_NAME testBookmarksMatch
    LINK_LIBRARIES Qt::Test krunner_bookmarks_common
)
