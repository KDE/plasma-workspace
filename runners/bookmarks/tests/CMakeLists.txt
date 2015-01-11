
include(ECMMarkAsTest)


set( testChromeBookmarks_SRCS testchromebookmarks.cpp 
  ../browsers/chrome.cpp
  ../faviconfromblob.cpp
  ../browsers/chromefindprofile.cpp
  ../bookmarkmatch.cpp
  ../favicon.cpp
  ../fetchsqlite.cpp
)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/.. )
add_executable( testChromeBookmarks ${testChromeBookmarks_SRCS} )
#add_executable( testChromeBookmarksNAME plasma-runner-bookmarks-TestChromeBookmarks ${testChromeBookmarks_SRCS} )
add_test(testChromeBookmarks testChromeBookmarks)
ecm_mark_as_test(testChromeBookmarks)
#add_test( testChromeBookmarks TESTNAME plasma-runner-bookmarks-TestChromeBookmarks ${testChromeBookmarks_SRCS} )
target_link_libraries( testChromeBookmarks
    Qt5::Gui
    Qt5::Test
    Qt5::Sql
    KF5::Runner
    KF5::KIOCore
)

add_test(plasma-runner-bookmarks-TestChromeBookmarks testChromeBookmarks)
ecm_mark_as_test(testChromeBookmarks)

file(COPY chrome-config-home DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
