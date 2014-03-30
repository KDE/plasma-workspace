project(plasma-runner-kill)

set(krunner_kill_SRCS killrunner.cpp)

set(kcm_krunner_kill_SRCS
    killrunner_config.cpp
)

qt5_wrap_ui(kcm_krunner_kill_SRCS killrunner_config.ui)
add_library(kcm_krunner_kill MODULE ${kcm_krunner_kill_SRCS})
target_link_libraries(kcm_krunner_kill
    
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KCMUTILS_LIBS} 
    Qt5::Core
    Qt5::Gui
    )
    
add_library(krunner_kill MODULE ${krunner_kill_SRCS})
target_link_libraries(krunner_kill
                      KF5::Plasma KIOCore KF5::ProcesCore)
add_dependencies(krunner_kill kcm_krunner_kill)

install(TARGETS krunner_kill kcm_krunner_kill
        DESTINATION ${PLUGIN_INSTALL_DIR})
        
install(FILES plasma-runner-kill.desktop plasma-runner-kill_config.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
