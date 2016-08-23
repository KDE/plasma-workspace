add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_weather\")

add_definitions(
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

add_subdirectory(ions)

set(weather_SRCS weatherengine.cpp)
ecm_qt_declare_logging_category(weather_SRCS
    HEADER weatherenginedebug.h
    IDENTIFIER WEATHER
    CATEGORY_NAME kde.dataengine.weather
    DEFAULT_SEVERITY Info
)

add_library(plasma_engine_weather MODULE ${weather_SRCS})

target_link_libraries (plasma_engine_weather
            KF5::KIOCore
            KF5::Solid
            KF5::Plasma
            Qt5::Network
            weather_ion)

kcoreaddons_desktop_to_json(plasma_engine_weather plasma-dataengine-weather.desktop SERVICE_TYPES plasma-dataengine.desktop)

install (TARGETS plasma_engine_weather DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/dataengine)
install (FILES plasma-dataengine-weather.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
