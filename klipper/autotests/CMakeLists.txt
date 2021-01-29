add_definitions(-DKLIPPER_UNIT_TEST)
include(ECMMarkAsTest)
########################################################
# Test History
########################################################
set(libklipper_test_SRCS)
ecm_qt_declare_logging_category(libklipper_test_SRCS HEADER klipper_debug.h IDENTIFIER KLIPPER_LOG CATEGORY_NAME org.kde.klipper)

set(testHistory_SRCS
    ${libklipper_test_SRCS}
    historytest.cpp
    ../history.cpp
    ../historyimageitem.cpp
    ../historyitem.cpp
    ../historystringitem.cpp
    ../historyurlitem.cpp
    ../historymodel.cpp
)
add_executable(testHistory ${testHistory_SRCS})
target_link_libraries(testHistory
    Qt::Test
    Qt::Widgets # QAction
    KF5::CoreAddons # KUrlMimeData
    KF5::I18n
)
add_test(NAME klipper-testHistory COMMAND testHistory)
ecm_mark_as_test(testHistory)

########################################################
# Test History Model
########################################################
set(testHistoryModel_SRCS
    historymodeltest.cpp
    ../historymodel.cpp
    ../historyimageitem.cpp
    ../historyitem.cpp
    ../historystringitem.cpp
    ../historyurlitem.cpp
    ${libklipper_test_SRCS}
)
add_executable(testHistoryModel ${testHistoryModel_SRCS})
target_link_libraries(testHistoryModel
    Qt::Test
    Qt::Widgets # QAction
    KF5::CoreAddons # KUrlMimeData
    KF5::I18n
)
add_test(NAME klipper-testHistoryModel COMMAND testHistoryModel)
ecm_mark_as_test(testHistoryModel)
