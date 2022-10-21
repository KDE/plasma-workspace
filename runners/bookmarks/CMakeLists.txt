########### next target ###############
#set(EXTERNAL_LIBS KIOCore Qt::Script KF5::Plasma Qt::Sql)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_bookmarksrunner\")

find_package(Qt${QT_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Sql)

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
    KF5::KIOGui
    KF5::I18n
    KF5::Runner
    KF5::Bookmarks
)

set_property(TARGET krunner_bookmarks_common PROPERTY POSITION_INDEPENDENT_CODE ON)

kcoreaddons_add_plugin(krunner_bookmarksrunner SOURCES bookmarksrunner.cpp browserfactory.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/krunner")
target_link_libraries(krunner_bookmarksrunner krunner_bookmarks_common)

# Currently tests include only chrome, so no need to get include them if json is not found
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
