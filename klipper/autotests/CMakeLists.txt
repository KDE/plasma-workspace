include(ECMMarkAsTest)

# Test History Cycler
ecm_add_test(historycyclertest.cpp TEST_NAME klipper-testHistoryCycler
    LINK_LIBRARIES Qt::Test klipper)

# Test History Model
add_executable(testHistoryModel historymodeltest.cpp)
target_link_libraries(testHistoryModel
    Qt::Test
    klipper
)
add_test(NAME klipper-testHistoryModel COMMAND testHistoryModel)
ecm_mark_as_test(testHistoryModel)

# Test Utils
add_executable(testKlipperUtils utilstest.cpp)
target_link_libraries(testKlipperUtils
    Qt::Test
    klipper
)
add_test(NAME klipper-testUtils COMMAND testKlipperUtils)
ecm_mark_as_test(testKlipperUtils)
# Klipper test
ecm_add_test(klippertest.cpp TEST_NAME klippertest
    LINK_LIBRARIES Qt::Test KF6::KIOCore klipper)

add_test(
    NAME klipper_v3migrationtest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/v3migrationtest.py --failfast
)
set_tests_properties(klipper_v3migrationtest PROPERTIES TIMEOUT 30 ENVIRONMENT "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")


