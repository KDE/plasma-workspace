include_directories(
        ${Boost_INCLUDE_DIR}
        ${KDEPIMLIBS_INCLUDE_DIRS}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set(akonadi_engine_srcs
    akonadiengine.cpp
)

add_library(plasma_engine_akonadi MODULE ${akonadi_engine_srcs})

target_link_libraries(
    plasma_engine_akonadi
    ${KDE4_KPIMUTILS_LIBS}
    KF5::CoreAddons
    ${KDE4_AKONADI_LIBS}
    KF5::Plasma
    ${KDE4_KMIME_LIBS}
    ${KDE4_AKONADI_KMIME_LIBS}
    ${KDE4_KABC_LIBS}
    Qt5::Xml
    microblog
)

install(TARGETS plasma_engine_akonadi DESTINATION ${PLUGIN_INSTALL_DIR}/plasma/dataengine)
install(FILES plasma-engine-akonadi.desktop DESTINATION ${SERVICES_INSTALL_DIR})

