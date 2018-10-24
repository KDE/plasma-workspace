
include(ECMAddTests)

ecm_add_test(testchromebookmarks.cpp TEST_NAME testChromeBookmarks
    LINK_LIBRARIES Qt5::Test krunner_bookmarks_test
)

file(COPY chrome-config-home DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
