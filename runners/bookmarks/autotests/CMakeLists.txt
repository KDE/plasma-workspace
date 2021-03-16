
include(ECMAddTests)

ecm_add_test(chrome/testchromebookmarks.cpp TEST_NAME testChromeBookmarks
    LINK_LIBRARIES Qt::Test krunner_bookmarks_test
)
file(COPY chrome/chrome-config-home DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/chrome/)
