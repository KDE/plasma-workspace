add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1410)

set(bugzillalibtest_SRCS
    bugzillalibtest.cpp 
    ../../bugzillaintegration/bugzillalib.cpp
)

add_executable(bugzillalibtest ${bugzillalibtest_SRCS})
ecm_mark_as_test(bugzillalibtest)

target_link_libraries(bugzillalibtest KF5::KIOWidgets KF5::XmlRpcClient Qt5::Xml)

