include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(krunner_powerdevil_SRCS
    PowerDevilRunner.cpp
)

add_library(krunner_powerdevil MODULE ${krunner_powerdevil_SRCS})
target_link_libraries(krunner_powerdevil ${KDE4_KDEUI_LIBS} KIOCore KF5::Plasma KF5::Solid)

install(TARGETS krunner_powerdevil DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-runner-powerdevil.desktop DESTINATION ${SERVICES_INSTALL_DIR})

