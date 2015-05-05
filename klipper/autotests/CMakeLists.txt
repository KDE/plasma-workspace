add_definitions(-DKLIPPER_UNIT_TEST)
include(ECMMarkAsTest)
########################################################
# Test History
########################################################
set(testHistory_SRCS
    historytest.cpp
    ../history.cpp
    ../historyimageitem.cpp
    ../historyitem.cpp
    ../historystringitem.cpp
    ../historyurlitem.cpp
    ../historymodel.cpp
    ../klipper_debug.cpp
)
add_executable(testHistory ${testHistory_SRCS})
target_link_libraries(testHistory
    Qt5::Test
    Qt5::Widgets # QAction
    KF5::CoreAddons # KUrlMimeData
)
add_test(klipper-testHistory testHistory)
ecm_mark_as_test(testHistory)

########################################################
# Test History Model
########################################################
set(testHistoryModel_SRCS
    historymodeltest.cpp
    modeltest.cpp
    ../historymodel.cpp
    ../historyimageitem.cpp
    ../historyitem.cpp
    ../historystringitem.cpp
    ../historyurlitem.cpp
    ../klipper_debug.cpp
)
add_executable(testHistoryModel ${testHistoryModel_SRCS})
target_link_libraries(testHistoryModel
    Qt5::Test
    Qt5::Widgets # QAction
    KF5::CoreAddons # KUrlMimeData
)
add_test(klipper-testHistoryModel testHistoryModel)
ecm_mark_as_test(testHistoryModel)
