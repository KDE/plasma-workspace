########### next target ###############
#set(EXTERNAL_LIBS KIOCore Qt5::Script KF5::Plasma Qt5::Sql)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_runner_bookmarksrunner\")

find_package(Qt5 CONFIG REQUIRED COMPONENTS Sql)


set(EXTERNAL_LIBS
    Qt5::Sql
    KF5::KIOCore
    KF5::I18n
    KF5::Runner
    KF5::Bookmarks
)

set(krunner_bookmarksrunner_SRCS
    browserfactory.cpp
    bookmarkmatch.cpp
    faviconfromblob.cpp
    favicon.cpp
    fetchsqlite.cpp
    browsers/opera.cpp
    bookmarksrunner.cpp
    browsers/kdebrowser.cpp
    browsers/firefox.cpp
)

set(krunner_bookmarksrunner_SRCS
    ${krunner_bookmarksrunner_SRCS}
    browsers/chromefindprofile.cpp
    browsers/chrome.cpp
)


add_library(krunner_bookmarksrunner MODULE ${krunner_bookmarksrunner_SRCS})
target_link_libraries(krunner_bookmarksrunner ${EXTERNAL_LIBS})

install(TARGETS krunner_bookmarksrunner DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install(FILES plasma-runner-bookmarks.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

# Currently tests include only chrome, so no need to get include them if json is not found
add_subdirectory(tests)
