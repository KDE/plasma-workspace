add_executable(kfontprint_bin Printer.cpp)
set_target_properties(kfontprint_bin PROPERTIES OUTPUT_NAME kfontprint)

target_link_libraries(kfontprint_bin
    Qt::PrintSupport
    KF6::IconThemes
    kfontinstui
    kfontinst
)

install(TARGETS kfontprint_bin DESTINATION ${KDE_INSTALL_LIBEXECDIR} )

