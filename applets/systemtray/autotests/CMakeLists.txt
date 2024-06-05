ecm_add_tests(systemtraymodeltest.cpp
    LINK_LIBRARIES systemtraymodel_static
    Qt::Test
)

set(statusnotifiertest_SRCS
    statusnotifiertest.cpp
)
add_executable(systemtray_statusnotifiertest ${statusnotifiertest_SRCS})
target_link_libraries(systemtray_statusnotifiertest
    Qt::Core
    Qt::Widgets
    KF6::CoreAddons
    KF6::StatusNotifierItem
)
