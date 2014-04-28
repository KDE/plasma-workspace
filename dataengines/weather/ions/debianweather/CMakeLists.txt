# Project Needs a name ofcourse
project(iondebianweather)
 
# We add our source code here
set(karaboudjan_SRCS iondebianweather.cpp)
 
# Now make sure all files get to the right place
add_library(ion_debianweather MODULE ${karaboudjan_SRCS})
target_link_libraries(ion_debianweather weather_ion
                      Qt5::Core)
 
install(TARGETS ion_debianweather
       DESTINATION ${PLASMA_DATA_INSTALL_DIR}/dataengine)
 
install(FILES ion-debianweather.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
