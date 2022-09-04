add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_weather\")
remove_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x50f02)

add_definitions(
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

add_subdirectory(ions)

set(weather_SRCS weatherengine.cpp weatherengine.h)
ecm_qt_declare_logging_category(weather_SRCS
    HEADER weatherenginedebug.h
    IDENTIFIER WEATHER
    CATEGORY_NAME kde.dataengine.weather
    DEFAULT_SEVERITY Info
)

kcoreaddons_add_plugin(plasma_engine_weather SOURCES ${weather_SRCS} INSTALL_NAMESPACE plasma/dataengine)
target_compile_definitions(plasma_engine_weather PRIVATE -DQT_DISABLE_DEPRECATED_BEFORE=0x050e00) # needed for QNetworkConfigurationManager

target_link_libraries (plasma_engine_weather
            KF5::KIOCore
            KF5::Solid
            KF5::Plasma
            Qt::Network
            weather_ion)
