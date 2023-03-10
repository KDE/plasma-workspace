include(ECMMarkAsTest)

# Test History
add_executable(testHistory historytest.cpp)
target_link_libraries(testHistory
    Qt::Test
    libklipper_common_static
)
add_test(NAME klipper-testHistory COMMAND testHistory)
ecm_mark_as_test(testHistory)

# Test History Model
add_executable(testHistoryModel historymodeltest.cpp)
target_link_libraries(testHistoryModel
    Qt::Test
    libklipper_common_static
)
add_test(NAME klipper-testHistoryModel COMMAND testHistoryModel)
ecm_mark_as_test(testHistoryModel)

# Test Utils
add_executable(testKlipperUtils utilstest.cpp)
target_link_libraries(testKlipperUtils
    Qt::Test
    libklipper_common_static
)
add_test(NAME klipper-testUtils COMMAND testKlipperUtils)
ecm_mark_as_test(testKlipperUtils)

# Klipper test
ecm_add_test(klippertest.cpp TEST_NAME klippertest
    LINK_LIBRARIES Qt::Test libklipper_common_static)

