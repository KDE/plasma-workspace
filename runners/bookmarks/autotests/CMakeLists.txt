
include(ECMAddTests)

ecm_add_test(chrome/testchromebookmarks.cpp TEST_NAME testChromeBookmarks
    LINK_LIBRARIES Qt::Test  krunner_bookmarks_common
)
file(COPY chrome/chrome-config-home DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/chrome/)

ecm_add_test(firefox/testfirefoxbookmarks.cpp TEST_NAME testFirefoxBookmarks
    LINK_LIBRARIES Qt::Test krunner_bookmarks_common
)