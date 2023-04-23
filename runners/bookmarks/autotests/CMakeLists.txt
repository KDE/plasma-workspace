# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

include(ECMAddTests)

ecm_add_tests(chrome/testchromebookmarks.cpp firefox/testfirefoxbookmarks.cpp bookmarksmatchtest.cpp
    LINK_LIBRARIES Qt::Test krunner_bookmarks_common
)

