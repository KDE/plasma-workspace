# SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-2-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_bookmarksrunner\")

find_package(Qt6 CONFIG REQUIRED COMPONENTS Sql)

add_library(krunner_bookmarks_common STATIC)
set_property(TARGET krunner_bookmarks_common PROPERTY POSITION_INDEPENDENT_CODE ON)

ecm_qt_declare_logging_category(krunner_bookmarks_common
    HEADER bookmarks_debug.h
    IDENTIFIER RUNNER_BOOKMARKS
    CATEGORY_NAME org.kde.plasma.runner.bookmarks
    DEFAULT_SEVERITY Warning
)

target_sources(krunner_bookmarks_common PRIVATE
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
target_link_libraries(krunner_bookmarks_common
    Qt::Sql
    KF6::KIOCore
    KF6::KIOGui
    KF6::I18n
    KF6::Runner
    KF6::Bookmarks
)

kcoreaddons_add_plugin(krunner_bookmarksrunner SOURCES bookmarksrunner.cpp browserfactory.cpp INSTALL_NAMESPACE "kf6/krunner")
target_link_libraries(krunner_bookmarksrunner krunner_bookmarks_common)

# Currently tests include only chrome, so no need to get include them if json is not found
if(BUILD_TESTING)
   add_subdirectory(autotests)
endif()
