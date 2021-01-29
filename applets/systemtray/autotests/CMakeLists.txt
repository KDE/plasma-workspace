include(ECMAddTests)

set(systemtraymodel_test_SRCS
    ../dbusserviceobserver.cpp
    ../plasmoidregistry.cpp
    ../systemtraysettings.cpp
    ../systemtraymodel.cpp
    ../sortedsystemtraymodel.cpp
)

ecm_qt_declare_logging_category(systemtraymodel_test_SRCS HEADER debug.h
    IDENTIFIER SYSTEM_TRAY
    CATEGORY_NAME kde.systemtray
    DEFAULT_SEVERITY Info)

add_library(systemtraymodel_test STATIC ${systemtraymodel_test_SRCS})

target_link_libraries(systemtraymodel_test
    Qt::Core
    Qt::DBus
    Qt::Quick
    KF5::CoreAddons
    KF5::I18n
    KF5::ItemModels
    KF5::Plasma
    KF5::XmlGui
)

ecm_add_tests(systemtraymodeltest.cpp
    LINK_LIBRARIES systemtraymodel_test
    Qt::Test
)
