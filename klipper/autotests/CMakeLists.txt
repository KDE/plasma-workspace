include(ECMMarkAsTest)

# Test History Cycler
ecm_add_test(historycyclertest.cpp TEST_NAME klipper-testHistoryCycler
    LINK_LIBRARIES Qt::Test klipper)

# nested KWin doesn't work on FreeBSD CI
if (LINUX)
    # Test History Model
    add_executable(testHistoryModel historymodeltest.cpp)
    target_link_libraries(testHistoryModel
        Qt::Test
        klipper
    )
    add_test(NAME klipper-testHistoryModel COMMAND dbus-run-session --config-file=${CMAKE_CURRENT_SOURCE_DIR}/isolated-session-bus.conf kwin_wayland --virtual --exit-with-session=$<TARGET_FILE:testHistoryModel>)
    ecm_mark_as_test(testHistoryModel)

    # Klipper test
    add_executable(klippertest klippertest.cpp)
    target_link_libraries(klippertest PRIVATE klipper Qt::Test KF6::Service KF6::ConfigCore)

    add_test(NAME klippertest COMMAND dbus-run-session --config-file=${CMAKE_CURRENT_SOURCE_DIR}/isolated-session-bus.conf kwin_wayland --virtual --exit-with-session=$<TARGET_FILE:klippertest>)

    # Clipboard deadlock regression test
    add_executable(clipboarddeadlocktest clipboarddeadlocktest.cpp)
    target_link_libraries(clipboarddeadlocktest PRIVATE Qt::Test Qt::Widgets KF6::GuiAddons)

    add_test(NAME clipboarddeadlocktest COMMAND dbus-run-session --config-file=${CMAKE_CURRENT_SOURCE_DIR}/isolated-session-bus.conf kwin_wayland --virtual --exit-with-session=$<TARGET_FILE:clipboarddeadlocktest>)
    set_tests_properties(clipboarddeadlocktest PROPERTIES TIMEOUT 60)
endif()

# Test Utils
add_executable(testKlipperUtils utilstest.cpp)
target_link_libraries(testKlipperUtils
    Qt::Test
    klipper
)
add_test(NAME klipper-testUtils COMMAND testKlipperUtils)
ecm_mark_as_test(testKlipperUtils)

add_test(
    NAME klipper_v3migrationtest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/v3migrationtest.py --failfast
)
set_tests_properties(klipper_v3migrationtest PROPERTIES ENVIRONMENT "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")


