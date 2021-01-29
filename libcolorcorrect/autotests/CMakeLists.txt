include(ECMAddTests)

set(ColorCorrectTest_SRCS nightcolortest.cpp mock_kwin.cpp)

ecm_add_test(
    ${ColorCorrectTest_SRCS}
    TEST_NAME nightcolortest
    LINK_LIBRARIES colorcorrect Qt::Test Qt::DBus
)
