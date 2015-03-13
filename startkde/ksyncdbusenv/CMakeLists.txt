add_executable(ksyncdbusenv ksyncdbusenv.cpp)
ecm_mark_nongui_executable(ksyncdbusenv)
target_link_libraries(ksyncdbusenv Qt5::Core Qt5::DBus)

install(TARGETS ksyncdbusenv DESTINATION ${KDE_INSTALL_LIBEXECDIR})
