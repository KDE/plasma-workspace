set(fontinst_bin_SRCS FcConfig.cpp FontInst.cpp Folder.cpp Main.cpp Utils.cpp ../dbus/FontinstIface.cpp)
qt_add_dbus_adaptor(fontinst_bin_SRCS org.kde.fontinst.xml FontInst.h KFI::FontInst)

add_executable(fontinst_bin ${fontinst_bin_SRCS})

set_target_properties(fontinst_bin PROPERTIES OUTPUT_NAME fontinst)
target_link_libraries(fontinst_bin 
    Qt::DBus
    Qt::Xml
    KF6::KIOCore
    kfontinst
)

ecm_qt_declare_logging_category(fontinst_bin
    HEADER kfontinst_debug.h
    IDENTIFIER KFONTINST_DEBUG
    CATEGORY_NAME org.kde.plasma.kfontinst
    DESCRIPTION "kcm kfontinst"
    EXPORT PLASMAWORKSPACE
)

install(TARGETS fontinst_bin DESTINATION ${KDE_INSTALL_LIBEXECDIR})
install(PROGRAMS fontinst_x11 DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(org.kde.fontinst.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/session/org.kde.fontinst.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/session/org.kde.fontinst.service DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR} )
