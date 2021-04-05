########### next target ###############
#set(EXTERNAL_LIBS KIOCore Qt::Script KF5::Plasma Qt::Sql)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_bookmarksrunner\")

find_package(Qt5 CONFIG REQUIRED COMPONENTS Sql)

set(krunner_bookmarks_common_SRCS
    bookmarkmatch.cpp
    faviconfromblob.cpp
    favicon.cpp
    fetchsqlite.cpp
    browsers/chromefindprofile.cpp
    browsers/chrome.cpp
    browsers/firefox.cpp
    browsers/konqueror.cpp
    browsers/opera.cpp
    browsers/falkon.cpp
)

ecm_qt_declare_logging_category(krunner_bookmarks_common_SRCS
    HEADER bookmarks_debug.h
    IDENTIFIER RUNNER_BOOKMARKS
    CATEGORY_NAME org.kde.plasma.runner.bookmarks
    DEFAULT_SEVERITY Warning
)

add_library(krunner_bookmarks_common STATIC ${krunner_bookmarks_common_SRCS})
target_link_libraries(krunner_bookmarks_common
    Qt::Sql
    KF5::KIOCore
    KF5::I18n
    KF5::Runner
    KF5::Bookmarks
)

add_library(krunner_bookmarksrunner MODULE bookmarksrunner.cpp browserfactory.cpp)
kcoreaddons_desktop_to_json(krunner_bookmarksrunner plasma-runner-bookmarks.desktop )
target_link_libraries(krunner_bookmarksrunner krunner_bookmarks_common)
set_target_properties(krunner_bookmarksrunner PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/krunner")

install(TARGETS krunner_bookmarksrunner DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")

# Currently tests include only chrome, so no need to get include them if json is not found
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
