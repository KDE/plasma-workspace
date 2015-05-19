add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_rss\")

include_directories(${Boost_INCLUDE_DIR} ${KDEPIMLIBS_INCLUDE_DIR})

set(rss_engine_SRCS
    rss.cpp
)

add_library(plasma_engine_rss MODULE ${rss_engine_SRCS})
target_link_libraries(plasma_engine_rss KF5::Plasma KF5::CoreAddons ${KDE4_SYNDICATION_LIBS} KF5::Solid)

install(TARGETS plasma_engine_rss DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install(FILES plasma-dataengine-rss.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

